#include "InteractionCore/CameraController.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <osg/View>
#include <osgText/Font>
#include <osgText/Text>
#include <osg/Vec4>
#include <osg/Program>
#include <osg/Geometry>
#include <osg/PrimitiveSetIndirect>
#include <QObject>
#include "mxr/base/log.h"
#include <InteractionCore/Tool/InteractionToolContext.h>

// 匿名空间，仅用于当前文件的debug的to string 函数
namespace {
    std::string to_string(const osg::Matrix& m)
    {
        std::ostringstream oss;
        // 设定小数点后 6 位精度，并使用定点格式
        oss << std::fixed << std::setprecision(6);
        for (int row = 0; row < 4; ++row)
        {
            oss << "[ ";
            for (int col = 0; col < 4; ++col)
            {
                // 每个数宽度 10，右对齐
                oss << std::setw(10) << m(row, col);
                if (col < 3) oss << ", ";
            }
            oss << " ]";
            if (row < 3) oss << "\n";
        }
        return oss.str();
    }
    std::string to_string(const osg::Vec4& v, int precision = 6)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        oss << "( "
            << std::setw(0) << v.x() << ", "
            << std::setw(0) << v.y() << ", "
            << std::setw(0) << v.z() << ", "
            << std::setw(0) << v.w()
            << " )";
        return oss.str();
    }
    std::string to_string(const osg::Vec3& v, int precision = 6)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        oss << "( "
            << std::setw(0) << v.x() << ", "
            << std::setw(0) << v.y() << ", "
            << std::setw(0) << v.z()
            << " )";
        return oss.str();
    }
    std::string to_string(const osg::Vec2& v, int precision = 6)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        oss << "( "
            << std::setw(0) << v.x() << ", "
            << std::setw(0) << v.y()
            << " )";
        return oss.str();
    }

    class ZeroRadiusSphereFinder : public osg::NodeVisitor
    {
    public:
        // 构造时指定遍历所有子节点
        ZeroRadiusSphereFinder()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {}

        // 每遇到一个节点就检查它的 bounding sphere
        virtual void apply(osg::Node& node) override
        {
            const osg::BoundingSphere& bs = node.getBound();
            if (bs.radius() == 0.0f)
            {
                _zeroSphereNodes.push_back(&node);
            }
            // 继续遍历子节点
            traverse(node);
        }

        std::vector<osg::ref_ptr<osg::Node>> _zeroSphereNodes;
    };
}

namespace mxr {

    CameraController::CameraController()
    {
        _timer = std::make_shared<QTimer>();
    }

    void CameraController::applyStateToCamera()
    {
        osg::Camera* camera = _saved_viewer->asView()->getCamera();
        double aspectRatio = camera->getViewport()->aspectRatio();
        _currentState.aspect_ratio = aspectRatio;
        _currentState.world_per_pixel = _currentState.ortho_zoom * 2 * _currentState.window_height_inverse;
        _currentState.defaultPixelSize = camera->getViewport()->width() * 0.05;
        _currentState.eye_center_distance = (_currentState.eye - _currentState.center).length();
        _currentState.eye_dir = _currentState.center - _currentState.eye;
        _currentState.eye_dir.normalize();

        //MXR_LOG_DEBUG("pixel: %f\n", _currentState.world_per_pixel);

        // 更新view矩阵
        if (_currentState.control_type == CameraState::ControlType::MoveCenter)
        {
            // view 矩阵计算
            _currentState.view_mat = osg::Matrix::lookAt(_currentState.eye, _currentState.center, _currentState.up);
        }
        else
        {
            // view 矩阵计算
            _currentState.view_mat = osg::Matrix::lookAt(_currentState.eye, _currentState.center, _currentState.up);
        }
        if (_currentState.projection_type == CameraState::ProjectionType::PERSPECTIVE)
        {
            // proj 矩阵计算
            _currentState.proj_mat = osg::Matrix::perspective(osg::RadiansToDegrees(_currentState.fov), aspectRatio, _currentState.z_near, _currentState.z_far);
        }
        else
        {
            // ortho 矩阵计算
            float halfHeight = _currentState.ortho_zoom;
            float halfWidth = halfHeight * aspectRatio;

            osg::Vec2 left_right = { -halfWidth - _currentState.pan_offset.x(), halfWidth - _currentState.pan_offset.x() };
            osg::Vec2 down_top = { -halfHeight - _currentState.pan_offset.y(), halfHeight - _currentState.pan_offset.y() };

            osg::Vec3 bound_center = getBoundCenter() * _currentState.view_mat;
            //std::cout << "bound center: " << to_string(bound_center) << "\n";
            //std::cout << "center: " << to_string(_currentState.center * _currentState.view_mat) << "\n";

            double final_near = (- bound_center.length() - getBoundRadius()) * 2.0f;
            double final_far = (+ bound_center.length() + getBoundRadius()) * 2.0f;

            _currentState.z_near = final_near;
            _currentState.z_far = final_far;
            _currentState.proj_mat = osg::Matrix::ortho(left_right.x(), left_right.y(), down_top.x(), down_top.y(), final_near, final_far);

            // 计算无缩放无位移投影矩阵
            halfHeight = _currentState.ortho_zoom_default;
            halfWidth = halfHeight * aspectRatio;
            _currentState.proj_mat_noMove_noScale = osg::Matrix::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, final_near, final_far);
        }

        // 计算旋转四元数
        {
            osg::Matrix rotate;
            osg::Matrix viewRotation = _currentState.view_mat;
            viewRotation.setTrans(0, 0, 0); // 去除平移部分
            osg::Vec3 t, s;
            osg::Quat r, so;
            osg::Matrix::inverse(viewRotation).decompose(t, r, s, so);
            rotate.makeRotate(r);
            _currentState.view_rotate_inverse = rotate;
        }

        camera->setViewMatrix(_currentState.view_mat);
        camera->setProjectionMatrix(_currentState.proj_mat);

        //std::cout << "projection: " << to_string(_currentState.proj_mat) << "\n";
    }

    void CameraController::update()
    {
        osg::BoundingSphere cur_sphere;
        for (int i = 0; i < _needToCaculateAABBNodeArray.size(); i++)
        {
            const auto& sphere = _needToCaculateAABBNodeArray[i]->getBound();

            // 用于查找包围盒半径为0的节点
            //{
            //    ZeroRadiusSphereFinder sphereFinder;
            //    _needToCaculateAABBNodeArray[i]->accept(sphereFinder);
            //    for (auto& n : sphereFinder._zeroSphereNodes)
            //    {
            //        std::cout << "[SphereZero] " << n->getName()
            //            << " @ " << n.get() << std::endl;
            //    }
            //}
            cur_sphere.expandBy(_needToCaculateAABBNodeArray[i]->getBound());
        }
        _bound_sphere = cur_sphere;
        if (updateAnimation())
        {
            // 这里的aa实际上就是我们在mbaserend中初始化的osgRender，所以他既是viewer，又是qobject
            osgViewer::Viewer* pViewer = dynamic_cast<osgViewer::Viewer*>(_saved_viewer->asView());
            QObject* qobj = dynamic_cast<QObject*>(_saved_viewer->asView());
            if (_timer_ID < 0)
            {
                // 这里osgrender绑定了timeEvent是调用osgrender的update，实际上就是QOpenGLWidget的update，会强制更新绘制
                _timer_ID = qobj->startTimer(10, Qt::PreciseTimer);
            }
            // 直接设置scheme没用，因为viewer被重写了
            //pViewer->setRunFrameScheme(osgViewer::Viewer::CONTINUOUS);
            //pViewer->setRunMaxFrameRate(60);
            //aa.requestRedraw();
        }
        else
        {
            _saved_viewer->requestContinuousUpdate(false);
            osgViewer::Viewer* pViewer = dynamic_cast<osgViewer::Viewer*>(_saved_viewer->asView());
            QObject* qobj = dynamic_cast<QObject*>(_saved_viewer->asView());
            // 删除计时器，并且设置保存的ID为负，防止重复删除/设置计时器
            if (_timer_ID > 0)
            {
                qobj->killTimer(_timer_ID);
                _timer_ID = -1;
            }
            //pViewer->setRunFrameScheme(osgViewer::Viewer::ON_DEMAND);
            //pViewer->setRunMaxFrameRate(0);
        }
    }

    void CameraController::resize(const int width, const int height)
    {
        _currentState.window_width = width;
        _currentState.window_height = height;
        _currentState.window_width_inverse = 1.0f / _currentState.window_width;
        _currentState.window_height_inverse = 1.0f / _currentState.window_height;
    }


    bool CameraController::orbit(double dx, double dy)
    {
        stopAnimation();
        switch (_orbitMethod)
        {
        case OrbitMethod::SPHERICAL_COORDINATES: orbitSpherical(dx, dy); break;
        case OrbitMethod::VECTOR_MATRIX:         orbitVectorMatrix(dx, dy); break;
        case OrbitMethod::QUATERNION:            orbitQuaternion(dx, dy); break;
            //case OrbitMethod::TRACKBALL:             orbitTrackball(ea); break;
        }
        //std::cout << "eye: " << to_string(_currentState.eye) << "\n";
        //std::cout << "center: " << to_string(_currentState.center) << "\n";
        //std::cout << "up: " << to_string(_currentState.up) << "\n";

        return true; // Orbit always changes the view
    }

    void CameraController::orbitSpherical(double dx, double dy)
    {
        osg::Vec3 viewDir = _currentState.center - _currentState.eye;
        double radius = viewDir.length();
        double theta = atan2(viewDir.y(), viewDir.x());
        double phi = acos(viewDir.z() / radius);

        theta -= dx * 0.005;
        phi -= dy * 0.005;

        const double epsilon = 0.001;
        if (phi < epsilon) phi = epsilon;
        if (phi > osg::PI - epsilon) phi = osg::PI - epsilon;

        _currentState.eye.x() = _currentState.center.x() + radius * sin(phi) * cos(theta);
        _currentState.eye.y() = _currentState.center.y() + radius * sin(phi) * sin(theta);
        _currentState.eye.z() = _currentState.center.z() + radius * cos(phi);
    }

    void CameraController::orbitVectorMatrix(double dx, double dy)
    {
        osg::Vec3 viewDir = _currentState.center - _currentState.eye;
        osg::Vec3 side = viewDir ^ _currentState.up;
        side.normalize();

        osg::Matrixd rotMatrix1, rotMatrix2;
        rotMatrix1.makeRotate(dx * -0.005, _currentState.up);
        rotMatrix2.makeRotate(dy * -0.005, side);

        _currentState.eye = _currentState.center + viewDir * rotMatrix1 * rotMatrix2;
        _currentState.up = _currentState.up * rotMatrix1 * rotMatrix2;
    }

    void CameraController::orbitQuaternion(double dx, double dy)
    {
        if (_rotate_type == Rotate_XY)
        {
            if (_currentState.control_type == CameraState::ControlType::MoveCenter)
            {
                osg::Vec3 viewDir = _currentState.center - _currentState.eye;
                osg::Vec3 side = viewDir ^ _currentState.up;
                side.normalize();

                osg::Quat rot_x(dy * +0.005, side);
                osg::Quat rot_y(dx * -0.005, _currentState.up);
                osg::Quat total_rot = rot_y * rot_x;

                _currentState.eye = _currentState.center - total_rot * viewDir;
                _currentState.up = total_rot * _currentState.up;
            }
            else
            {
                // 正交投影下围绕原点旋转
                osg::Vec3 viewDir = _currentState.center - _currentState.eye;
                osg::Vec3 side = viewDir ^ _currentState.up;
                side.normalize();

                osg::Quat rot_x(dy * +0.005, side);
                osg::Quat rot_y(dx * -0.005, _currentState.up);
                osg::Quat total_rot = rot_y * rot_x;

                _currentState.eye = _currentState.center - total_rot * viewDir;
                _currentState.up = total_rot * _currentState.up;
            }
        }
        else if (_rotate_type == Rotate_Z)// 以相机朝向轴为中心旋转
        {
            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();
            osg::Quat rot_z(dx * 0.005, viewDir);

            _currentState.up = rot_z * _currentState.up;
        }
    }

    osg::Vec3 CameraController::mapToSphere(const osg::Vec2f& p)
    {
        osg::Vec3 v(p.x(), p.y(), 0);
        float d = v.length2();
        if (d <= _trackballSize * _trackballSize * 0.5f)
        {
            v.z() = sqrt(_trackballSize * _trackballSize - d);
        }
        else
        {
            v.z() = (_trackballSize * _trackballSize) / (2.0f * sqrt(d));
        }
        v.normalize();
        return v;
    }

    void CameraController::startTrackball(const osgGA::GUIEventAdapter& ea)
    {
        _trackballStartPos = mapToSphere(osg::Vec2f(ea.getXnormalized(), ea.getYnormalized()));
    }

    void CameraController::orbitTrackball(const osgGA::GUIEventAdapter& ea)
    {
        osg::Vec3 currentPos = mapToSphere(osg::Vec2f(ea.getXnormalized(), ea.getYnormalized()));
        osg::Vec3 axis = _trackballStartPos ^ currentPos;
        double angle = acos(std::min(1.0f, std::max(-1.0f, _trackballStartPos * currentPos))); // Clamp to avoid nan

        if (axis.length2() < 1e-6 || std::isnan(angle)) return;

        osg::Quat rotation;
        rotation.makeRotate(angle, axis);

        // 世界坐标系下的旋转
        osg::Matrixd cam_to_world;
        cam_to_world.makeLookAt(_currentState.eye, _currentState.center, _currentState.up);
        osg::Quat world_rotation = cam_to_world.getRotate() * rotation;

        osg::Vec3 viewDir = _currentState.center - _currentState.eye;
        _currentState.eye = _currentState.center + world_rotation * viewDir * getBoundRadius();
        _currentState.up = world_rotation * _currentState.up;

        // 更新起始点，以便连续拖动
        _trackballStartPos = currentPos;
    }

    bool CameraController::pan(double dx, double dy)
    {
        stopAnimation();

        if (_currentState.control_type == CameraState::ControlType::MoveCenter)
        {
            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            double distance = viewDir.length();
            double panFactor = distance * _pan_speed_persp;

            osg::Vec3 side = viewDir ^ _currentState.up;
            side.normalize();
            osg::Vec3 localUp = side ^ viewDir;
            localUp.normalize();

            osg::Vec3 offset = -(side * dx + localUp * dy) * panFactor;
            _currentState.eye += offset;
            _currentState.center += offset;

            //std::cout << "eye: " << to_string(_currentState.eye) << "\n";
            //std::cout << "center: " << to_string(_currentState.center) << "\n";
            //std::cout << "up: " << to_string(_currentState.up) << "\n";
        }
        else
        {
            // 在正交模式下，平移速度应与缩放级别（可视范围）相关
            // ortho_zoom 代表了可视区域一半的高度
            double panFactor = _currentState.ortho_zoom * _pan_speed_ortho;

            const osg::Vec3 x_dir = { 1.0f, 0.0f, 0.0f };
            const osg::Vec3 y_dir = { 0.0f, 1.0f, 0.0f };
            //osg::Vec3 offset = (x_dir * dx + y_dir * dy) * panFactor;
            //osg::Vec3 offset = (x_dir * dx * _currentState.window_width_inverse + y_dir * dy * _currentState.window_height_inverse) * _currentState.ortho_zoom * 2;
            osg::Vec3 offset = (x_dir * dx + y_dir * dy) * _currentState.ortho_zoom * 2 * _currentState.window_height_inverse;

            //std::cout << "offset: " << to_string(offset) << "\n";
            //osg::Vec3 offset = (x_dir * dx + y_dir * dy) * window_height_inverse * _currentState.ortho_zoom;
            _currentState.pan_offset += offset;
        }
        return true;
    }

    bool CameraController::zoom(const double motion, const float x, const float y)
    {
        stopAnimation();
        if (_currentState.projection_type == CameraState::ProjectionType::PERSPECTIVE)
        {
            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            double distance = viewDir.length();
            double zoomFactor = 1.0 - motion * 0.1;
            if (distance * zoomFactor < 0.1) return false;
            _currentState.eye = _currentState.center - viewDir * zoomFactor;
        }
        else
        {
#ifdef CENTER_TYPE
            double zoomFactor = 1.0 - motion * 0.1;
            _currentState.ortho_zoom *= zoomFactor;
            if (_currentState.ortho_zoom < 0.0001) _currentState.ortho_zoom = 0.0001;
            //MXR_LOG_DEBUG("zoom: %f\n", _currentState.ortho_zoom);
#else 
            double zoomFactor = 1.0 - motion * 0.1;

            // 将参考中心投影到屏幕以获取其深度值
            osg::Vec3 center_screen_3d = worldToScreen(_currentState.center);
            center_screen_3d.z() = std::min(1.0f, std::max(-1.0f, center_screen_3d.z()));

            osg::Vec3 cursor_pos{ x, y, center_screen_3d.z() };
            osg::Vec3 oldPanOffset = _currentState.pan_offset;

            osg::Vec3 screen_offset_old = (cursor_pos - worldToScreen(_currentState.center)) * _currentState.ortho_zoom * 2 * _currentState.window_height_inverse;
            // 正常缩放
            _currentState.ortho_zoom *= zoomFactor;
            applyStateToCamera();
            osg::Vec3 screen_offset_new = (cursor_pos - worldToScreen(_currentState.center)) * _currentState.ortho_zoom * 2 * _currentState.window_height_inverse;

            _currentState.pan_offset = oldPanOffset - screen_offset_old + screen_offset_new;
#endif
        }
        //MXR_LOG_INFO("zoom factor: %f", _currentState.ortho_zoom);
        return true;
    }

    bool CameraController::zoomPercentage(const double percent)
    {
        stopAnimation();
        CameraState target = _currentState;
        if (_currentState.projection_type == CameraState::ProjectionType::PERSPECTIVE)
        {
            osg::Vec3 viewDir = target.eye - target.center;
            double distance = viewDir.length();
            double zoomFactor = 100.0 / percent;
            if (distance * zoomFactor < 0.1) return false;
            target.eye = target.center + viewDir * zoomFactor;
        }
        else
        {
            target.ortho_zoom = getBoundRadius() * 100.0 / percent;
            if (target.ortho_zoom < 0.0001) target.ortho_zoom = 0.0001;
            MXR_LOG_DEBUG("zoom: %f\n", target.ortho_zoom);
        }

        startAnimation(target);
        // 保存视角快照
        pushViewStack(target);
        return true;
    }

    bool CameraController::rotateAxis(const float angle, ModeGeomTransf _axis_type, const osg::Vec3& _otherAxis)
    {
        
        switch (_axis_type)
        {
        case ModeGeomTransf::X_AXIS_PLUS:
        { 
            osg::Vec3 viewDir = _currentState.up ^ (_currentState.eye - _currentState.center);
			viewDir.normalize();
           
			osg::Quat rot_x(osg::DegreesToRadians(angle), viewDir);
            _currentState.eye = _currentState.center + rot_x * (_currentState.eye- _currentState.center);
            osg::Vec3 up = rot_x * _currentState.up; up.normalize();
            _currentState.up = rot_x * _currentState.up;
        }
            break;
		case ModeGeomTransf::X_AXIS_MINUS:
		{
            osg::Vec3 viewDir = (_currentState.eye - _currentState.center) ^ _currentState.up;
			viewDir.normalize();
			osg::Quat rot_x(osg::DegreesToRadians(angle), viewDir);
			_currentState.eye = _currentState.center + rot_x * (_currentState.eye - _currentState.center);
			osg::Vec3 up = rot_x * _currentState.up; up.normalize();
			_currentState.up = rot_x * _currentState.up;
		}
		break;
        case ModeGeomTransf::Y_AXIS_PLUS:
        {
            osg::Vec3 viewDir = (_currentState.eye - _currentState.center) ^ (_currentState.up ^ (_currentState.eye - _currentState.center));
			viewDir.normalize();
			osg::Quat rot_y(osg::DegreesToRadians(angle), viewDir);
            _currentState.eye = _currentState.center + rot_y * (_currentState.eye - _currentState.center);
        }
            break;
        case ModeGeomTransf::Y_AXIS_MINUS:
        {
            osg::Vec3 viewDir = (_currentState.up ^ (_currentState.eye - _currentState.center)) ^ (_currentState.eye - _currentState.center);
			viewDir.normalize();
			osg::Quat rot_y(osg::DegreesToRadians(angle), viewDir);
            _currentState.eye = _currentState.center + rot_y * (_currentState.eye - _currentState.center);
        }
			break;
        case ModeGeomTransf::Z_AXIS_PLUS:
        {
            osg::Vec3 viewDir = _currentState.eye - _currentState.center;
			viewDir.normalize();
			osg::Quat rot_z(osg::DegreesToRadians(angle), viewDir);
            osg::Vec3 up = rot_z * _currentState.up; up.normalize();
            _currentState.up = up;
        }
            break;
        case ModeGeomTransf::Z_AXIS_MINUS:
        {
			osg::Vec3 viewDir = _currentState.center- _currentState.eye;
			viewDir.normalize();
			osg::Quat rot_z(osg::DegreesToRadians(angle), viewDir);
			osg::Vec3 up = rot_z * _currentState.up ; up.normalize();
			_currentState.up = up;
        }
			break;
        case ModeGeomTransf::OTHER_AXIS:
        {
            osg::Quat rot(osg::DegreesToRadians(angle), _otherAxis);
			_currentState.eye = _currentState.center + rot*(_currentState.eye - _currentState.center);
			osg::Vec3 up = _currentState.center+rot * (_currentState.up- _currentState.center); up.normalize();
            _currentState.up = up;
        }
             break;
        default:
            break;
        }
        // 保存视角快照
        pushViewStack(_currentState);
        return true;
    }

    void CameraController::immediateToTargetState()
    {
        _currentState = _targetState;
        _isAnimating = false;
    }

    void CameraController::pushViewStack(const CameraState& view)
    {
        // 如果当前上一步是空的，则会保存上一次
        if (_lastStateStack.empty())
        {
            _lastStateStack.push_back(_startState);
        }

        if (_lastStateStack.size() < _stackMaxCount)
        {
            // 保存视角快照
            _lastStateStack.push_back(view);
        }
        else
        {
            _lastStateStack.pop_front();
            _lastStateStack.push_back(view);
        }

        // 清除重做队列
        _nextStateStack.clear();
    }

    void CameraController::lastView()
    {
        if (_lastStateStack.size() > 1)
        {
            // 取出视角快照
            const auto& state = _lastStateStack.back();
            // 保存视角快照到重做队列
            _nextStateStack.push_back(std::move(_lastStateStack.back()));
            _lastStateStack.pop_back();
        }
        if (!_lastStateStack.empty())
        {
            // 设置视角为当前堆栈顶部
            startAnimation(_lastStateStack.back());
        }
    }

    void CameraController::nextView()
    {
        if (!_nextStateStack.empty())
        {
            // 取出视角快照
            const auto& state = _nextStateStack.back();
            // 保存视角快照到重做队列
            _lastStateStack.push_back(std::move(_nextStateStack.back()));
            _nextStateStack.pop_back();
        }
        if (!_lastStateStack.empty())
        {
            // 设置视角为当前堆栈顶部
            startAnimation(_lastStateStack.back());
        }
    }


    void CameraController::fitToView(const float x, const float y, const float z, const float radius)
    {
        CameraState target = _currentState;

        if (target.projection_type == CameraState::ProjectionType::PERSPECTIVE)
        {
            target.center = { x, y, z };

            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();

            // 使用一个较小的最小视场角来防止fov为0时除以零
            double fov_rad = osg::DegreesToRadians(std::max(target.fov, 1.0));
            double dist_to_center = getBoundRadius();

            // 2. 确定新的相机位置，并增加一个边距让视野更舒适
            target.eye = target.center + viewDir * dist_to_center; // 增加50%的边距

            // 3. 从新的相机位置重新计算到物体中心的实际距离
            double final_dist_to_center = (target.eye - target.center).length();

            // 4. 根据包围球计算理想的近裁剪面和远裁剪面
            // 近裁剪面 = 到中心的距离 - 包围球半径
            //target.near = final_dist_to_center - getBoundRadius();
            // 远裁剪面 = 到中心的距离 + 包围球半径
            target.z_far = final_dist_to_center + radius * 10;

            // 5. 应用安全边距，并确保zNear是一个很小的正数
            target.z_near *= 0.9; // 将近平面再拉近一点，避免裁剪掉物体正面
            target.z_far *= 1.1; // 将远平面再推远一点，确保物体背面完整

            // 6. 健壮性检查：确保 near > 0 且 near < far
            if (target.z_near < 0.1)
            {
                target.z_near = 0.1;
            }
            if (target.z_far <= target.z_near)
            {
                // 如果计算结果不合理（例如模型非常小），提供一个后备方案
                target.z_far = target.z_near + getBoundRadius() * 2.2;
            }
        }
        else if (target.projection_type == CameraState::ProjectionType::ORTHOGRAPHIC)
        {
            target.center = { x, y, z };

            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();

            // 使用一个较小的最小视场角来防止fov为0时除以零
            double fov_rad = osg::DegreesToRadians(std::max(target.fov, 1.0));
            double dist_to_center = getBoundRadius();
            target.eye = target.center - viewDir * radius; // 增加50%的边距

            target.ortho_zoom = radius / 2;

            double clip_depth = std::min(getBoundRadius() * 2.0, 0.1);
            target.z_near = -clip_depth;
            target.z_far = clip_depth;
        }

        target.pan_offset = { 0.0f, 0.0f, 0.0f };
        startAnimation(target);
        // 保存视角快照
        pushViewStack(target);
    }

    void CameraController::updateSceneBoundingSphere(const osg::BoundingSphere& bs)
    {
        _bound_sphere = bs;
    }

    void CameraController::setView(Perspective type)
    {

        double distance = getBoundRadius() * 2.2;
        CameraState target = _currentState;

        if (_currentState.control_type == CameraState::ControlType::MoveCenter)
        {
            target.center = getBoundCenter();
            target.pan_offset = { 0.0f, 0.0f, 0.0f };
            switch (type)
            {
            case Perspective::Front:  target.eye = target.center + osg::Vec3(0, 0, +1) * distance; target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::Rear:   target.eye = target.center + osg::Vec3(0, 0, -1) * distance; target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::Top:    target.eye = target.center + osg::Vec3(0, -1, 0) * distance; target.up = osg::Vec3(0, 0, -1); break;
            case Perspective::Bottom: target.eye = target.center + osg::Vec3(0, +1, 0) * distance; target.up = osg::Vec3(0, 0, +1); break;
            case Perspective::Left:   target.eye = target.center + osg::Vec3(-1, 0, 0) * distance; target.up = osg::Vec3(0, 0, +1); break;
            case Perspective::Right:  target.eye = target.center + osg::Vec3(+1, 0, 0) * distance; target.up = osg::Vec3(0, 0, +1); break;
            case Perspective::ISO1:   target.eye = target.center + osg::Vec3(1, -1, 1) * distance; target.up = osg::Vec3(1, -1, -2);  break;
            case Perspective::ISO2:   target.eye = target.center + osg::Vec3(-1, -1, 1) * distance; target.up = osg::Vec3(-1, -1, -2); break;
            case Perspective::ISO3:   target.eye = target.center + osg::Vec3(1, 1, 1) * distance; target.up = osg::Vec3(1, 1, -2);   break;
            case Perspective::ISO4:   target.eye = target.center + osg::Vec3(-1, 1, 1) * distance; target.up = osg::Vec3(-1, 1, -2);  break;
            case Perspective::ISO5:   target.eye = target.center + osg::Vec3(+1, 0, 0) * distance; target.up = osg::Vec3(0, 0, 1); break;
            }
        }
        else if (_currentState.control_type == CameraState::ControlType::NoMoveCenter)
        {
            switch (type)
            {
            case Perspective::Front:  target.eye = osg::Vec3(0, 0, +1); target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::Rear:   target.eye = osg::Vec3(0, 0, -1); target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::Top:    target.eye = osg::Vec3(0, +1, 0); target.up = osg::Vec3(0, 0, -1); break;
            case Perspective::Bottom: target.eye = osg::Vec3(0, -1, 0); target.up = osg::Vec3(0, 0, +1); break;
            case Perspective::Left:   target.eye = osg::Vec3(-1, 0, 0); target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::Right:  target.eye = osg::Vec3(+1, 0, 0); target.up = osg::Vec3(0, +1, 0); break;
            case Perspective::ISO1:   target.eye = osg::Vec3(+0.500000, +0.707107, +0.500000); target.up = osg::Vec3(-0.500000, +0.707107, -0.500000); break;
            case Perspective::ISO2:   target.eye = osg::Vec3(-0.500000, +0.707107, -0.500000); target.up = osg::Vec3(+0.500000, +0.707107, +0.500000); break;
            case Perspective::ISO3:   target.eye = osg::Vec3(-0.500000, -0.500000, +0.707107); target.up = osg::Vec3(+0.500000, +0.500000, +0.707107); break;
            case Perspective::ISO4:   target.eye = osg::Vec3(-0.500000, -0.707107, -0.500000); target.up = osg::Vec3(-0.500000, +0.707107, -0.500000); break;
            }
            target.eye *= distance;
            target.eye += target.center;
        }

        startAnimation(target);
        // 保存视角快照
        pushViewStack(target);
    }

    void CameraController::setProjection(CameraState::ProjectionType type)
    {
        stopAnimation();
        _currentState.projection_type = type;
    }

    osg::Vec3 CameraController::screenToWorld(const float x, const float y, const float z) const
    {
        // 拿到当前相机的视口
        osg::Camera* camera = _saved_viewer->asView()->getCamera();
        osg::Viewport* vp = camera->getViewport();
        //const auto window_mat = vp->computeWindowMatrix(); // osg实际上提供直接获得屏幕空间到裁剪空间的矩阵
        float vx = vp->x(), vy = vp->y();
        float vw = vp->width(), vh = vp->height();

        // 屏幕坐标映射到 [0,1]
        float u = (x - vx) / vw;
        float v = (y - vy) / vh;

        // 再映射到 NDC [-1,1]（注意：OpenGL 下 Y 方向通常要翻转）
        float ndcX = 2.0f * u - 1.0f;
        float ndcY = 1.0f - 2.0f * v;
        float ndcZ = 2.0f * z - 1.0f;//默认情况下(自定义除外)屏幕空间中深度Z值被映射到[0,1]------需要进行变换
       
        // 齐次裁剪坐标
        osg::Vec4 clipV(ndcX, ndcY, ndcZ, 1.0f);
        osg::Matrix invVP = osg::Matrix::inverse(getViewMatrix() * getPerspectiveMatrix()); // 逆矩阵

        // 把裁剪坐标逆变换回世界齐次坐标
        osg::Vec4 worldHom = clipV * invVP;

        // 再除以 w 得到真正的三维世界坐标
        osg::Vec3 worldPos(worldHom.x() / worldHom.w(), worldHom.y() / worldHom.w(), worldHom.z() / worldHom.w());

        return worldPos;
    }

    osg::Vec3 CameraController::worldToScreen(const osg::Vec3& worldPoint) const
    {
        const osg::Matrix& viewMatrix = getViewMatrix();
        const osg::Matrix& projectionMatrix = getPerspectiveMatrix();
        const osg::Viewport* viewport = _saved_viewer->asView()->getCamera()->getViewport();

       // osg::Matrix vpwMatrix = viewMatrix * projectionMatrix * viewport->computeWindowMatrix();
        //在视口变换前需要做一次透视除法---需要用4维齐次坐标
        osg::Matrix vpwMatrix = viewMatrix * projectionMatrix;
        osg::Vec4 screenPoint = osg::Vec4(worldPoint,1.0f) * vpwMatrix;
        screenPoint /= screenPoint.w();
       
        auto mat = viewport->computeWindowMatrix();
        mat(1, 1) *= -1.0f;//viewport->computeWindowMatrix()得出的矩阵中关于Y轴的变换是正的，需要调整
        screenPoint = screenPoint * mat;
        return { std::roundf(screenPoint.x()),std::roundf(screenPoint.y()),screenPoint.z() };//计算可能出现小数，需要进行取整
    }

    mxr::Ray CameraController::makeRay(const float screen_x, const float screen_y) const
    {
        // 将像素坐标转到归一化设备坐标 (NDC)
        float ndcX = 2.0f * screen_x * _currentState.window_width_inverse - 1.0f;
        float ndcY = 1.0f - 2.0f * screen_y * _currentState.window_height_inverse; // Y 轴翻转
        //MXR_LOG_DEBUG("ndc x: %.3f y: %.3f\n", ndcX, ndcY);
        osg::Matrix modify_view_mat = _currentState.view_mat;
        modify_view_mat.setTrans(osg::Vec3{ 0,0,0 });
        osg::Matrix inv = osg::Matrix::inverse(_currentState.view_mat * _currentState.proj_mat);

        osg::Vec4 nearW = osg::Vec4(ndcX, ndcY, -1.0f, 1.0f) * inv;
        osg::Vec4 farW = osg::Vec4(ndcX, ndcY, 1.0f, 1.0f) * inv;
        osg::Vec3 near_point = osg::Vec3(nearW.x(), nearW.y(), nearW.z()) / nearW.w();
        osg::Vec3 far_point = osg::Vec3(farW.x(), farW.y(), farW.z()) / farW.w();

        osg::Vec3 dir = far_point - near_point;
        dir.normalize();
        return mxr::Ray(near_point, dir);
    }

    void CameraController::setRotateCenter(const float x, const float y, const float z, const float radius)
    {
        if (_currentState.control_type == CameraState::ControlType::MoveCenter)
        {
            osg::Vec3d viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();

            // 更新旋转中心
            CameraState target = _currentState;
            target.center = { x, y, z };

            // 根据新的中心、观察方向和期望距离，计算出新的相机位置
            target.eye = target.center + viewDir * radius;

            // 重置2D平移量，因为我们已经改变了3D视图
            target.pan_offset.set(0, 0, 0);

            startAnimation(target);
        }
        else
        {
            osg::Vec3d viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();
            // 更新旋转中心
            CameraState target = _currentState;
            target.center = { x, y, z };
            target.eye = target.center - viewDir * radius;

            // 计算新旧中心在相机空间下的位移差并添加到平移记录中，这样保证了显示的物体不会移动
            osg::Vec3 offset = target.center * osg::Matrix::lookAt(_currentState.eye, _currentState.center, _currentState.up);
            target.pan_offset += offset;
            startAnimation(target);
            //std::cout << "center: " << to_string(target.center) << "\n";
        }
        // 保存视角快照
        pushViewStack(_currentState);
    }

    void CameraController::setRotateScreenCenter()
    {
        osg::Vec3 screen_center{ _currentState.window_width / 2.0f, _currentState.window_height / 2.0f, 0.0f };
        screen_center = screenToWorld(screen_center.x(), screen_center.y(), screen_center.z());
        screen_center.z() = _currentState.center.z();
        float distance = (_currentState.eye - _currentState.center).length();
        setRotateCenter(screen_center.x(), screen_center.y(), screen_center.z(), distance);
        // 保存视角快照
        pushViewStack(_currentState);
    }

    void CameraController::fitToRotateCenter(const float x, const float y, const float z, const float radius)
    {
        CameraState target = _currentState;
        if (_currentState.control_type == CameraState::ControlType::MoveCenter)
        {
            osg::Vec3d viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();

            // 2. 更新旋转中心
            target.center = { x, y, z };

            // 3. 根据新的中心、观察方向和期望距离，计算出新的相机位置
            target.eye = target.center + viewDir * radius;

            // 4. 重置2D平移量，因为我们已经改变了3D视图
            target.pan_offset.set(0, 0, 0);

        }
        else
        {
            // 更新旋转中心
            //_currentState.center = { x, y, z };
            target.center = { x, y, z };

            // 计算新旧中心在相机空间下的位移差并添加到平移记录中，这样保证了显示的物体不会移动
            osg::Vec3 offset = target.center * osg::Matrix::lookAt(_currentState.eye, _currentState.center, _currentState.up);
            target.pan_offset += offset;
            //_currentState = target;
            //std::cout << "center: " << to_string(target.center) << "\n";
        }
        startAnimation(target);
        //if (!_bound_sphere.valid()) return;

        if (target.projection_type == CameraState::ProjectionType::PERSPECTIVE)
        {
            target.center = _currentState.center;

            osg::Vec3 viewDir = _currentState.center - _currentState.eye;
            viewDir.normalize();

            // 使用一个较小的最小视场角来防止fov为0时除以零
            double fov_rad = osg::DegreesToRadians(std::max(target.fov, 1.0));
            double dist_to_center = getBoundRadius();

            // 2. 确定新的相机位置，并增加一个边距让视野更舒适
            target.eye = target.center + viewDir * dist_to_center; // 增加50%的边距

            // 3. 从新的相机位置重新计算到物体中心的实际距离
            double final_dist_to_center = (target.eye - target.center).length();

            // 4. 根据包围球计算理想的近裁剪面和远裁剪面
            // 近裁剪面 = 到中心的距离 - 包围球半径
            // 远裁剪面 = 到中心的距离 + 包围球半径
            target.z_far = final_dist_to_center + getBoundRadius() * 10;

            // 5. 应用安全边距，并确保zNear是一个很小的正数
            target.z_near *= 0.9; // 将近平面再拉近一点，避免裁剪掉物体正面
            target.z_far *= 1.1; // 将远平面再推远一点，确保物体背面完整

            // 6. 健壮性检查：确保 near > 0 且 near < far
            if (target.z_near < 0.1)
            {
                target.z_near = 0.1;
            }
            if (target.z_far <= target.z_near)
            {
                // 如果计算结果不合理（例如模型非常小），提供一个后备方案
                target.z_far = target.z_near + getBoundRadius() * 2.2;
            }
        }
        else if (target.projection_type == CameraState::ProjectionType::ORTHOGRAPHIC)
        {
            target.center = _currentState.center;

            target.ortho_zoom = getBoundRadius();

            double clip_depth = getBoundRadius() * 2.0;
            target.z_near = -clip_depth;
            target.z_far = clip_depth;
        }

        target.pan_offset = { 0.0f, 0.0f, 0.0f };
        startAnimation(target);
        // 保存视角快照
        pushViewStack(target);
    }

    void CameraController::saveCameraState()
    {
        _savedState = _currentState;
    }

    void CameraController::saveTargetState()
    {
        _savedState = _targetState;
    }

    void CameraController::recoverCameraState()
    {
        _savedState.pan_offset = _currentState.pan_offset;
        _savedState.ortho_zoom = _currentState.ortho_zoom;
        startAnimation(_savedState);
        // 保存视角快照
        pushViewStack(_savedState);
    }

    void CameraController::startAnimation(const CameraState& targetState)
    {
        _isAnimating = true;
        _startState = _currentState;
        _targetState = targetState;
        _animationStartTime = osg::Timer::instance()->time_s();

        // 计算距离
        _startState.eye_center_distance = (_startState.eye - _startState.center).length();
        _targetState.eye_center_distance = (_targetState.eye - _targetState.center).length();

        // 计算旋转四元数 (最关键的一步)
        // osg::Matrix::lookAt 会创建一个视图矩阵
        // 视图矩阵的逆矩阵就是相机在世界中的模型矩阵
        // 我们可以从这个模型矩阵中提取出旋转四元数
        osg::Matrix start_view_matrix = osg::Matrix::lookAt(_startState.eye, _startState.center, _startState.up);
        osg::Matrix end_view_matrix = osg::Matrix::lookAt(_targetState.eye, _targetState.center, _targetState.up);

        start_rotation = start_view_matrix.getRotate().inverse();
        end_rotation = end_view_matrix.getRotate().inverse();

        // 确保Slerp走最短路径
        // 如果两个四元数的点乘为负，说明它们指向相反的半球，
        // Slerp会绕远路。反转其中一个可以解决这个问题。
        if (start_rotation.asVec4() * end_rotation.asVec4() < 0.0)
        {
            end_rotation = -end_rotation;
        }

        _isAnimating = true;
        QTimer::singleShot(0, [this] {update();});
    }


    bool CameraController::updateAnimation()
    {
        // 传入一个引用来更新
        if (!_isAnimating) return false;

        double elapsedTime = osg::Timer::instance()->time_s() - _animationStartTime;
        float t = std::min(elapsedTime / _animationDuration, 1.0);

        // 使用平滑步进函数，让动画有缓入缓出效果
        t = t * t * (3.0f - 2.0f * t);

        // 插值平移和缩放 (线性插值 lerp)
        _currentState.center = _startState.center * (1.0 - t) + _targetState.center * t;
        _currentState.pan_offset = _startState.pan_offset * (1.0 - t) + _targetState.pan_offset * t;
        _currentState.ortho_zoom = _startState.ortho_zoom * (1.0 - t) + _targetState.ortho_zoom * t;
        double current_distance = _startState.eye_center_distance * (1.0 - t) + _targetState.eye_center_distance * t;

        // 插值旋转 (球面线性插值 s-lerp)
        osg::Quat current_rotation;
        current_rotation.slerp(t, start_rotation, end_rotation);

        // 从插值结果中重建 eye 和 up 向量
        // 默认的视线方向是 -Z 轴
        osg::Vec3d view_direction(0, 0, -1);
        // 将这个方向向量用我们插值得到的旋转进行变换
        osg::Vec3d rotated_view_direction = current_rotation.operator*(view_direction);
        // 根据新的中心点、旋转方向和距离，计算出新的eye点
        _currentState.eye = _currentState.center - rotated_view_direction * current_distance;

        // 默认的up方向是 +Y 轴
        osg::Vec3d up_direction(0, 1, 0);
        // 同样用插值得到的旋转进行变换，得到新的up向量
        _currentState.up = current_rotation.operator*(up_direction);

        // 检查动画是否结束
        if (elapsedTime >= _animationDuration)
        {
            _isAnimating = false;
             _currentState = _targetState;
        }
        return true;
    }

    void CameraController::stopAnimation()
    {
        if (_isAnimating)
        {
            _isAnimating = false;
            //_currentState = _targetState;
            applyStateToCamera();
        }
    }

    void CameraController::registeAABBNode(std::vector<osg::Node*> node_array)
    {
        for (int i = 0; i < node_array.size(); i++)
        {
            _needToCaculateAABBNodeArray.push_back(node_array[i]);
        }
    }

    //bool CameraController::updateAnimation()
    //{
    //    if (!_isAnimating) return false;
    //    //if (!_viewer)
    //    //{
    //    //	_isAnimating = false;
    //    //	return false;
    //    //}
    //    double currentTime = osg::Timer::instance()->time_s();
    //    double t = (currentTime - _animationStartTime) / _animationDuration;

    //    if (t >= 1.0)
    //    {
    //        stopAnimation();
    //        //_viewer->requestContinuousUpdate(false);
    //    }
    //    else
    //    {
    //        t = t * t * (3.0f - 2.0f * t);

    //        _currentState.eye = _startState.eye * (1.0 - t) + _targetState.eye * t;
    //        _currentState.center = _startState.center * (1.0 - t) + _targetState.center * t;
    //        _currentState.pan_offset = _startState.pan_offset * (1.0 - t) + _targetState.pan_offset * t;

    //        osg::Quat start_q, interp_q;
    //        start_q.makeRotate(_startState.up, _targetState.up);
    //        interp_q.slerp(t, osg::Quat(), start_q);
    //        _currentState.up = interp_q * _startState.up;

    //        _currentState.fov = _startState.fov * (1.0 - t) + _targetState.fov * t;
    //        _currentState.ortho_zoom = _startState.ortho_zoom * (1.0 - t) + _targetState.ortho_zoom * t;
    //    }
    //    return true;
    //}



}