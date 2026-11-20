#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

class QualityCheckContext;

namespace MechMainWindow
{
    // 辅助函数：更新一对值的最大最小值
    inline void updateMinMax(std::pair<double, double>& target, const std::pair<double, double>& source) {
        if (source.first < target.first) target.first = source.first;
        if (source.second > target.second) target.second = source.second;
    }
    // 辅助函数：初始化一对值为极端值
    inline void initMinMax(std::pair<double, double>& p) {
        p.first = std::numeric_limits<double>::max();   // 最小值初始化为最大可能值
        p.second = std::numeric_limits<double>::lowest(); // 最大值初始化为最小可能值
    }
    struct QualityMaxMin {

        //最大最小值
        pair<double,double> edge_ratio;
        pair<double,double> length;
        pair<double,double> aspect_ratio;
        pair<double,double> aspect_frobenius;
        pair<double,double> radius_ratio;
        pair<double,double> area;
        pair<double,double> normalized_inradius;
        pair<double,double> relative_size_squared;
        pair<double,double> max_edge_ratio;
        pair<double,double> med_aspect_frobenius;
        pair<double,double> max_aspect_frobenius;
        pair<double,double> aspect_gamma;
        pair<double,double> volume;
        pair<double,double> diagonal;
        pair<double,double> dimension;
        pair<double,double> mean_aspect_frobenius;
        pair<double,double> minimum_angle;
        pair<double,double> maximum_angle;
        pair<double,double> shape;
        pair<double,double> equiangle_skew;
        pair<double,double> shape_and_size;
        pair<double,double> skew;
        pair<double,double> taper;
        pair<double,double> warpage;
        pair<double,double> stretch;
        pair<double,double> shear;
        pair<double,double> shear_and_size;
        pair<double,double> collapse_ratio;
        pair<double,double> equivolume_skew;
        pair<double,double> squish_index;
        pair<double,double> max_stretch;
        pair<double,double> mean_ratio;
        pair<double,double> condition;
        pair<double,double> scaled_jacobian;
        pair<double,double> distortion;
        pair<double,double> jacobian;
        pair<double,double> oddy;
        pair<double,double> nodal_jacobian_ratio;
        pair<double,double> enhanced_orth;
        //平均值
        double edge_ratio_mean =std::numeric_limits<double>::max();
        double length_mean = std::numeric_limits<double>::max();
        double aspect_ratio_mean = std::numeric_limits<double>::max();
        double aspect_frobenius_mean = std::numeric_limits<double>::max();
        double radius_ratio_mean = std::numeric_limits<double>::max();
        double area_mean = std::numeric_limits<double>::max();
        double normalized_inradius_mean = std::numeric_limits<double>::max();
        double relative_size_squared_mean = std::numeric_limits<double>::max();
        double max_edge_ratio_mean = std::numeric_limits<double>::max();
        double med_aspect_frobenius_mean = std::numeric_limits<double>::max();
        double max_aspect_frobenius_mean = std::numeric_limits<double>::max();
        double aspect_gamma_mean = std::numeric_limits<double>::max();
        double volume_mean = std::numeric_limits<double>::max();
        double diagonal_mean = std::numeric_limits<double>::max();
        double dimension_mean = std::numeric_limits<double>::max();
        double mean_aspect_frobenius_mean = std::numeric_limits<double>::max();
        double minimum_angle_mean = std::numeric_limits<double>::max();
        double maximum_angle_mean = std::numeric_limits<double>::max();
        double shape_mean = std::numeric_limits<double>::max();
        double equiangle_skew_mean = std::numeric_limits<double>::max();
        double shape_and_size_mean = std::numeric_limits<double>::max();
        double skew_mean = std::numeric_limits<double>::max();
        double taper_mean = std::numeric_limits<double>::max();
        double warpage_mean = std::numeric_limits<double>::max();
        double stretch_mean = std::numeric_limits<double>::max();
        double shear_mean = std::numeric_limits<double>::max();
        double shear_and_size_mean = std::numeric_limits<double>::max();
        double collapse_ratio_mean = std::numeric_limits<double>::max();
        double equivolume_skew_mean = std::numeric_limits<double>::max();
        double squish_index_mean = std::numeric_limits<double>::max();
        double max_stretch_mean = std::numeric_limits<double>::max();
        double mean_ratio_mean = std::numeric_limits<double>::max();
        double condition_mean = std::numeric_limits<double>::max();
        double scaled_jacobian_mean = std::numeric_limits<double>::max();
        double distortion_mean = std::numeric_limits<double>::max();
        double jacobian_mean = std::numeric_limits<double>::max();
        double oddy_mean = std::numeric_limits<double>::max();
        double nodal_jacobian_ratio_mean = std::numeric_limits<double>::max();
        double enhanced_orth_mean = std::numeric_limits<double>::max();
        QualityMaxMin() {
            initAll();
        }
        void initAll() {
            initMinMax(edge_ratio);
            initMinMax(length);
            initMinMax(aspect_ratio);
            initMinMax(aspect_frobenius);
            initMinMax(radius_ratio);
            initMinMax(area);
            initMinMax(normalized_inradius);
            initMinMax(relative_size_squared);
            initMinMax(max_edge_ratio);
            initMinMax(med_aspect_frobenius);
            initMinMax(max_aspect_frobenius);
            initMinMax(aspect_gamma);
            initMinMax(volume);
            initMinMax(diagonal);
            initMinMax(dimension);
            initMinMax(mean_aspect_frobenius);
            initMinMax(minimum_angle);
            initMinMax(maximum_angle);
            initMinMax(shape);
            initMinMax(equiangle_skew);
            initMinMax(shape_and_size);
            initMinMax(skew);
            initMinMax(taper);
            initMinMax(warpage);
            initMinMax(stretch);
            initMinMax(shear);
            initMinMax(shear_and_size);
            initMinMax(collapse_ratio);
            initMinMax(equivolume_skew);
            initMinMax(squish_index);
            initMinMax(max_stretch);
            initMinMax(mean_ratio);
            initMinMax(condition);
            initMinMax(scaled_jacobian);
            initMinMax(distortion);
            initMinMax(jacobian);
            initMinMax(oddy);
            initMinMax(nodal_jacobian_ratio);
            initMinMax(enhanced_orth);

            //LineMaxMin 初始化
            initMinMax(lineMaxMin.length);
            // TriMaxMin 初始化
            initMinMax(triMaxMin.edge_ratio);
            initMinMax(triMaxMin.aspect_ratio);
            initMinMax(triMaxMin.radius_ratio);
            initMinMax(triMaxMin.aspect_frobenius);
            initMinMax(triMaxMin.area);
            initMinMax(triMaxMin.minimum_angle);
            initMinMax(triMaxMin.maximum_angle);
            initMinMax(triMaxMin.condition);
            initMinMax(triMaxMin.scaled_jacobian);
            initMinMax(triMaxMin.shape);
            initMinMax(triMaxMin.distortion);
            initMinMax(triMaxMin.equiangle_skew);
            initMinMax(triMaxMin.normalized_inradius);
            initMinMax(triMaxMin.relative_size_squared);
            initMinMax(triMaxMin.shape_and_size);
            initMinMax(triMaxMin.enhanced_orth);
            // QuadMaxMin 初始化
            initMinMax(quadMaxMin.edge_ratio);
            initMinMax(quadMaxMin.max_edge_ratio);
            initMinMax(quadMaxMin.aspect_ratio);
            initMinMax(quadMaxMin.radius_ratio);
            initMinMax(quadMaxMin.med_aspect_frobenius);
            initMinMax(quadMaxMin.max_aspect_frobenius);
            initMinMax(quadMaxMin.skew);
            initMinMax(quadMaxMin.taper);
            initMinMax(quadMaxMin.warpage);
            initMinMax(quadMaxMin.area);
            initMinMax(quadMaxMin.stretch);
            initMinMax(quadMaxMin.minimum_angle);
            initMinMax(quadMaxMin.maximum_angle);
            initMinMax(quadMaxMin.oddy);
            initMinMax(quadMaxMin.condition);
            initMinMax(quadMaxMin.jacobian);
            initMinMax(quadMaxMin.scaled_jacobian);
            initMinMax(quadMaxMin.shear);
            initMinMax(quadMaxMin.shape);
            initMinMax(quadMaxMin.distortion);
            initMinMax(quadMaxMin.equiangle_skew);
            initMinMax(quadMaxMin.relative_size_squared);
            initMinMax(quadMaxMin.shape_and_size);
            initMinMax(quadMaxMin.shear_and_size);
            initMinMax(quadMaxMin.enhanced_orth);
            // TetMaxMin 初始化
            initMinMax(tetMaxMin.edge_ratio);
            initMinMax(tetMaxMin.radius_ratio);
            initMinMax(tetMaxMin.aspect_ratio);
            initMinMax(tetMaxMin.aspect_gamma);
            initMinMax(tetMaxMin.aspect_frobenius);
            initMinMax(tetMaxMin.minimum_angle);
            initMinMax(tetMaxMin.collapse_ratio);
            initMinMax(tetMaxMin.volume);
            initMinMax(tetMaxMin.condition);
            initMinMax(tetMaxMin.jacobian);
            initMinMax(tetMaxMin.scaled_jacobian);
            initMinMax(tetMaxMin.mean_ratio);
            initMinMax(tetMaxMin.normalized_inradius);
            initMinMax(tetMaxMin.shape);
            initMinMax(tetMaxMin.distortion);
            initMinMax(tetMaxMin.equivolume_skew);
            initMinMax(tetMaxMin.squish_index);
            initMinMax(tetMaxMin.equiangle_skew);
            initMinMax(tetMaxMin.relative_size_squared);
            initMinMax(tetMaxMin.shape_and_size);
            initMinMax(tetMaxMin.enhanced_orth);
            // HexMaxMin 初始化
            initMinMax(hexMaxMin.edge_ratio);
            initMinMax(hexMaxMin.max_edge_ratio);
            initMinMax(hexMaxMin.skew);
            initMinMax(hexMaxMin.taper);
            initMinMax(hexMaxMin.volume);
            initMinMax(hexMaxMin.stretch);
            initMinMax(hexMaxMin.diagonal);
            initMinMax(hexMaxMin.dimension);
            initMinMax(hexMaxMin.oddy);
            initMinMax(hexMaxMin.med_aspect_frobenius);
            initMinMax(hexMaxMin.max_aspect_frobenius);
            initMinMax(hexMaxMin.condition);
            initMinMax(hexMaxMin.jacobian);
            initMinMax(hexMaxMin.scaled_jacobian);
            initMinMax(hexMaxMin.shear);
            initMinMax(hexMaxMin.shape);
            initMinMax(hexMaxMin.distortion);
            initMinMax(hexMaxMin.equiangle_skew);
            initMinMax(hexMaxMin.nodal_jacobian_ratio);
            initMinMax(hexMaxMin.relative_size_squared);
            initMinMax(hexMaxMin.shape_and_size);
            initMinMax(hexMaxMin.shear_and_size);
            initMinMax(hexMaxMin.enhanced_orth);
            // WedgeMaxMin 初始化
            initMinMax(wedgeMaxMin.volume);
            initMinMax(wedgeMaxMin.edge_ratio);
            initMinMax(wedgeMaxMin.max_aspect_frobenius);
            initMinMax(wedgeMaxMin.mean_aspect_frobenius);
            initMinMax(wedgeMaxMin.jacobian);
            initMinMax(wedgeMaxMin.distortion);
            initMinMax(wedgeMaxMin.max_stretch);
            initMinMax(wedgeMaxMin.scaled_jacobian);
            initMinMax(wedgeMaxMin.shape);
            initMinMax(wedgeMaxMin.condition);
            initMinMax(wedgeMaxMin.equiangle_skew);
            initMinMax(wedgeMaxMin.enhanced_orth);
            // PyramidMaxMin 初始化
            initMinMax(pyramidMaxMin.volume);
            initMinMax(pyramidMaxMin.jacobian);
            initMinMax(pyramidMaxMin.scaled_jacobian);
            initMinMax(pyramidMaxMin.shape);
            initMinMax(pyramidMaxMin.equiangle_skew);
        }
        struct LineMaxMin
        {
            pair<double, double> length;

            double length_mean = std::numeric_limits<double>::max();
            size_t num;
        }lineMaxMin;
        struct TriMaxMin
        {
            pair<double, double> edge_ratio;
            pair<double, double> aspect_ratio;
            pair<double, double> radius_ratio;
            pair<double, double> aspect_frobenius;
            pair<double, double> area;
            pair<double, double> minimum_angle;
            pair<double, double> maximum_angle;
            pair<double, double> condition;
            pair<double, double> scaled_jacobian;
            pair<double, double> shape;
            pair<double, double> distortion;
            pair<double, double> equiangle_skew;
            pair<double, double> normalized_inradius;
            pair<double, double> relative_size_squared;
            pair<double, double> shape_and_size;
            pair<double, double> enhanced_orth;

            double edge_ratio_mean = std::numeric_limits<double>::max();
            double aspect_ratio_mean = std::numeric_limits<double>::max();
            double radius_ratio_mean = std::numeric_limits<double>::max();
            double aspect_frobenius_mean = std::numeric_limits<double>::max();
            double area_mean = std::numeric_limits<double>::max();
            double minimum_angle_mean = std::numeric_limits<double>::max();
            double maximum_angle_mean = std::numeric_limits<double>::max();
            double condition_mean = std::numeric_limits<double>::max();
            double scaled_jacobian_mean = std::numeric_limits<double>::max();
            double shape_mean = std::numeric_limits<double>::max();
            double distortion_mean = std::numeric_limits<double>::max();
            double equiangle_skew_mean = std::numeric_limits<double>::max();
            double normalized_inradius_mean = std::numeric_limits<double>::max();
            double relative_size_squared_mean = std::numeric_limits<double>::max();
            double shape_and_size_mean = std::numeric_limits<double>::max();
            double enhanced_orth_mean = std::numeric_limits<double>::max();

            size_t num;
        }triMaxMin;
        struct QuadMaxMin
        {
            pair<double, double> edge_ratio;
            pair<double, double> max_edge_ratio;
            pair<double, double> aspect_ratio;
            pair<double, double> radius_ratio;
            pair<double, double> med_aspect_frobenius;
            pair<double, double> max_aspect_frobenius;
            pair<double, double> skew;
            pair<double, double> taper;
            pair<double, double> warpage;
            pair<double, double> area;
            pair<double, double> stretch;
            pair<double, double> minimum_angle;
            pair<double, double> maximum_angle;
            pair<double, double> oddy;
            pair<double, double> condition;
            pair<double, double> jacobian;
            pair<double, double> scaled_jacobian;
            pair<double, double> shear;
            pair<double, double> shape;
            pair<double, double> distortion;
            pair<double, double> equiangle_skew;
            pair<double, double> relative_size_squared;
            pair<double, double> shape_and_size;
            pair<double, double> shear_and_size;
            pair<double, double> enhanced_orth;

            double edge_ratio_mean = std::numeric_limits<double>::max();
            double max_edge_ratio_mean = std::numeric_limits<double>::max();
            double aspect_ratio_mean = std::numeric_limits<double>::max();
            double radius_ratio_mean = std::numeric_limits<double>::max();
            double med_aspect_frobenius_mean = std::numeric_limits<double>::max();
            double max_aspect_frobenius_mean = std::numeric_limits<double>::max();
            double skew_mean= std::numeric_limits<double>::max();
            double taper_mean= std::numeric_limits<double>::max();
            double warpage_mean= std::numeric_limits<double>::max();
            double area_mean= std::numeric_limits<double>::max();
            double stretch_mean= std::numeric_limits<double>::max();
            double minimum_angle_mean= std::numeric_limits<double>::max();
            double maximum_angle_mean= std::numeric_limits<double>::max();
            double oddy_mean= std::numeric_limits<double>::max();
            double condition_mean= std::numeric_limits<double>::max();
            double jacobian_mean= std::numeric_limits<double>::max();
            double scaled_jacobian_mean= std::numeric_limits<double>::max();
            double shear_mean= std::numeric_limits<double>::max();
            double shape_mean= std::numeric_limits<double>::max();
            double distortion_mean= std::numeric_limits<double>::max();
            double equiangle_skew_mean= std::numeric_limits<double>::max();
            double relative_size_squared_mean= std::numeric_limits<double>::max();
            double shape_and_size_mean= std::numeric_limits<double>::max();
            double shear_and_size_mean= std::numeric_limits<double>::max();
            double enhanced_orth_mean = std::numeric_limits<double>::max();

            size_t num;
        }quadMaxMin;
        struct TetMaxMin
        {
            pair<double, double> edge_ratio;
            pair<double, double> radius_ratio;
            pair<double, double> aspect_ratio;
            pair<double, double> aspect_gamma;
            pair<double, double> aspect_frobenius;
            pair<double, double> minimum_angle;
            pair<double, double> collapse_ratio;
            pair<double, double> volume;
            pair<double, double> condition;
            pair<double, double> jacobian;
            pair<double, double> scaled_jacobian;
            pair<double, double> mean_ratio;
            pair<double, double> normalized_inradius;
            pair<double, double> shape;
            pair<double, double> distortion;
            pair<double, double> equivolume_skew;
            pair<double, double> squish_index;
            pair<double, double> equiangle_skew;
            pair<double, double> relative_size_squared;
            pair<double, double> shape_and_size;
            pair<double, double> enhanced_orth;

            double edge_ratio_mean= std::numeric_limits<double>::max();
            double radius_ratio_mean= std::numeric_limits<double>::max();
            double aspect_ratio_mean= std::numeric_limits<double>::max();
            double aspect_gamma_mean= std::numeric_limits<double>::max();
            double aspect_frobenius_mean= std::numeric_limits<double>::max();
            double minimum_angle_mean= std::numeric_limits<double>::max();
            double collapse_ratio_mean= std::numeric_limits<double>::max();
            double volume_mean= std::numeric_limits<double>::max();
            double condition_mean= std::numeric_limits<double>::max();
            double jacobian_mean= std::numeric_limits<double>::max();
            double scaled_jacobian_mean= std::numeric_limits<double>::max();
            double mean_ratio_mean= std::numeric_limits<double>::max();
            double normalized_inradius_mean= std::numeric_limits<double>::max();
            double shape_mean= std::numeric_limits<double>::max();
            double distortion_mean= std::numeric_limits<double>::max();
            double equivolume_skew_mean= std::numeric_limits<double>::max();
            double squish_index_mean= std::numeric_limits<double>::max();
            double equiangle_skew_mean= std::numeric_limits<double>::max();
            double relative_size_squared_mean= std::numeric_limits<double>::max();
            double shape_and_size_mean= std::numeric_limits<double>::max();
            double enhanced_orth_mean = std::numeric_limits<double>::max();

            size_t num;
        }tetMaxMin;
        struct HexMaxMin
        {
            pair<double, double> edge_ratio;
            pair<double, double> max_edge_ratio;
            pair<double, double> skew;
            pair<double, double> taper;
            pair<double, double> volume;
            pair<double, double> stretch;
            pair<double, double> diagonal;
            pair<double, double> dimension;
            pair<double, double> oddy;
            pair<double, double> med_aspect_frobenius;
            pair<double, double> max_aspect_frobenius;
            pair<double, double> condition;
            pair<double, double> jacobian;
            pair<double, double> scaled_jacobian;
            pair<double, double> shear;
            pair<double, double> shape;
            pair<double, double> distortion;
            pair<double, double> equiangle_skew;
            pair<double, double> nodal_jacobian_ratio;
            pair<double, double> relative_size_squared;
            pair<double, double> shape_and_size;
            pair<double, double> shear_and_size;
            pair<double, double> enhanced_orth;

            double edge_ratio_mean= std::numeric_limits<double>::max();
            double max_edge_ratio_mean= std::numeric_limits<double>::max();
            double skew_mean= std::numeric_limits<double>::max();
            double taper_mean= std::numeric_limits<double>::max();
            double volume_mean= std::numeric_limits<double>::max();
            double stretch_mean= std::numeric_limits<double>::max();
            double diagonal_mean= std::numeric_limits<double>::max();
            double dimension_mean= std::numeric_limits<double>::max();
            double oddy_mean= std::numeric_limits<double>::max();
            double med_aspect_frobenius_mean= std::numeric_limits<double>::max();
            double max_aspect_frobenius_mean= std::numeric_limits<double>::max();
            double condition_mean= std::numeric_limits<double>::max();
            double jacobian_mean= std::numeric_limits<double>::max();
            double scaled_jacobian_mean= std::numeric_limits<double>::max();
            double shear_mean= std::numeric_limits<double>::max();
            double shape_mean= std::numeric_limits<double>::max();
            double distortion_mean= std::numeric_limits<double>::max();
            double equiangle_skew_mean= std::numeric_limits<double>::max();
            double nodal_jacobian_ratio_mean= std::numeric_limits<double>::max();
            double relative_size_squared_mean= std::numeric_limits<double>::max();
            double shape_and_size_mean= std::numeric_limits<double>::max();
            double shear_and_size_mean= std::numeric_limits<double>::max();
            double enhanced_orth_mean = std::numeric_limits<double>::max();

            size_t num;
        }hexMaxMin;
        struct WedgeMaxMin
        {
            pair<double, double> volume;
            pair<double, double> edge_ratio;
            pair<double, double> max_aspect_frobenius;
            pair<double, double> mean_aspect_frobenius;
            pair<double, double> jacobian;
            pair<double, double> distortion;
            pair<double, double> max_stretch;
            pair<double, double> scaled_jacobian;
            pair<double, double> shape;
            pair<double, double> condition;
            pair<double, double> equiangle_skew;
            pair<double, double> enhanced_orth;

            double volume_mean= std::numeric_limits<double>::max();
            double edge_ratio_mean= std::numeric_limits<double>::max();
            double max_aspect_frobenius_mean= std::numeric_limits<double>::max();
            double mean_aspect_frobenius_mean= std::numeric_limits<double>::max();
            double jacobian_mean= std::numeric_limits<double>::max();
            double distortion_mean= std::numeric_limits<double>::max();
            double max_stretch_mean= std::numeric_limits<double>::max();
            double scaled_jacobian_mean= std::numeric_limits<double>::max();
            double shape_mean= std::numeric_limits<double>::max();
            double condition_mean= std::numeric_limits<double>::max();
            double equiangle_skew_mean= std::numeric_limits<double>::max();
            double enhanced_orth_mean = std::numeric_limits<double>::max();

            size_t num;
        }wedgeMaxMin;
        struct PyramidMaxMin
        {
            pair<double, double> volume;
            pair<double, double> jacobian;
            pair<double, double> scaled_jacobian;
            pair<double, double> shape;
            pair<double, double> equiangle_skew;

            double volume_mean= std::numeric_limits<double>::max();
            double jacobian_mean= std::numeric_limits<double>::max();
            double scaled_jacobian_mean= std::numeric_limits<double>::max();
            double shape_mean= std::numeric_limits<double>::max();
            double equiangle_skew_mean= std::numeric_limits<double>::max();

            size_t num;
        }pyramidMaxMin;
    };

    struct MetricMinMax
    {
        QString min;
        QString max;
    };
    // 模板函数：聚合所有结构体
    class QualityAggregator {
    private:
        // 辅助结构：存储加权和与权重
        struct WeightedSum {
            double sum = 0.0;
            size_t count = 0;
            void add(double mean_value, size_t element_count) {
                if (mean_value != std::numeric_limits<double>::max() && element_count > 0) {
                    sum += mean_value * element_count;
                    count += element_count;
                }
            }
            double getAverage() const {
                return count > 0 ? sum / count : std::numeric_limits<double>::max();
            }
        };
        QualityMaxMin& qmm;
    public:
        // 计算所有38个加权平均值
        void computeQualityMaxMinAverages() {
            // 每个指标对应一个加权计算单元
            std::unordered_map<std::string, WeightedSum> metrics;
            // 辅助lambda：安全获取pair的大小
            //auto getSize = [](const auto& p) -> size_t {
                // 假设pair的size()方法返回元素数量
                // 如果pair是min/max值，size()可能表示该类型网格单元的数量
            //    return p.size();
            //    };
            //===== 线网格贡献 =====
            const auto& line = qmm.lineMaxMin;
            if (line.length_mean != std::numeric_limits<double>::max()) {
                metrics["length"].add(line.length_mean, line.num);
            }
            // ===== 三角形贡献 =====
            const auto& tri = qmm.triMaxMin;
            if (tri.edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["edge_ratio"].add(tri.edge_ratio_mean, tri.num);
            }
            if (tri.aspect_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_ratio"].add(tri.aspect_ratio_mean, tri.num);
            }
            if (tri.radius_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["radius_ratio"].add(tri.radius_ratio_mean, tri.num);
            }
            if (tri.aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_frobenius"].add(tri.aspect_frobenius_mean, tri.num);
            }
            if (tri.area_mean != std::numeric_limits<double>::max()) {
                metrics["area"].add(tri.area_mean, tri.num);
            }
            if (tri.minimum_angle_mean != std::numeric_limits<double>::max()) {
                metrics["minimum_angle"].add(tri.minimum_angle_mean, tri.num);
            }
            if (tri.maximum_angle_mean != std::numeric_limits<double>::max()) {
                metrics["maximum_angle"].add(tri.maximum_angle_mean, tri.num);
            }
            if (tri.condition_mean != std::numeric_limits<double>::max()) {
                metrics["condition"].add(tri.condition_mean, tri.num);
            }
            if (tri.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(tri.scaled_jacobian_mean, tri.num);
            }
            if (tri.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(tri.shape_mean, tri.num);
            }
            if (tri.distortion_mean != std::numeric_limits<double>::max()) {
                metrics["distortion"].add(tri.distortion_mean, tri.num);
            }
            if (tri.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(tri.equiangle_skew_mean, tri.num);
            }
            if (tri.normalized_inradius_mean != std::numeric_limits<double>::max()) {
                metrics["normalized_inradius"].add(tri.normalized_inradius_mean, tri.num);
            }
            if (tri.relative_size_squared_mean != std::numeric_limits<double>::max()) {
                metrics["relative_size_squared"].add(tri.relative_size_squared_mean, tri.num);
            }
            if (tri.shape_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shape_and_size"].add(tri.shape_and_size_mean, tri.num);
            }
            if (tri.enhanced_orth_mean != std::numeric_limits<double>::max()) {
                metrics["enhanced_orth"].add(tri.enhanced_orth_mean, tri.num);
            }
            // ===== 四边形贡献 =====
            const auto& quad = qmm.quadMaxMin;
            if (quad.edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["edge_ratio"].add(quad.edge_ratio_mean, quad.num);
            }
            if (quad.max_edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["max_edge_ratio"].add(quad.max_edge_ratio_mean, quad.num);
            }
            if (quad.aspect_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_ratio"].add(quad.aspect_ratio_mean, quad.num);
            }
            if (quad.radius_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["radius_ratio"].add(quad.radius_ratio_mean, quad.num);
            }
            if (quad.med_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["med_aspect_frobenius"].add(quad.med_aspect_frobenius_mean, quad.num);
            }
            if (quad.max_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["max_aspect_frobenius"].add(quad.max_aspect_frobenius_mean, quad.num);
            }
            if (quad.skew_mean != std::numeric_limits<double>::max()) {
                metrics["skew"].add(quad.skew_mean, quad.num);
            }
            if (quad.taper_mean != std::numeric_limits<double>::max()) {
                metrics["taper"].add(quad.taper_mean, quad.num);
            }
            if (quad.warpage_mean != std::numeric_limits<double>::max()) {
                metrics["warpage"].add(quad.warpage_mean, quad.num);
            }
            if (quad.area_mean != std::numeric_limits<double>::max()) {
                metrics["area"].add(quad.area_mean, quad.num);
            }
            if (quad.stretch_mean != std::numeric_limits<double>::max()) {
                metrics["stretch"].add(quad.stretch_mean, quad.num);
            }
            if (quad.minimum_angle_mean != std::numeric_limits<double>::max()) {
                metrics["minimum_angle"].add(quad.minimum_angle_mean, quad.num);
            }
            if (quad.maximum_angle_mean != std::numeric_limits<double>::max()) {
                metrics["maximum_angle"].add(quad.maximum_angle_mean, quad.num);
            }
            if (quad.oddy_mean != std::numeric_limits<double>::max()) {
                metrics["oddy"].add(quad.oddy_mean, quad.num);
            }
            if (quad.condition_mean != std::numeric_limits<double>::max()) {
                metrics["condition"].add(quad.condition_mean, quad.num);
            }
            if (quad.jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["jacobian"].add(quad.jacobian_mean, quad.num);
            }
            if (quad.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(quad.scaled_jacobian_mean, quad.num);
            }
            if (quad.shear_mean != std::numeric_limits<double>::max()) {
                metrics["shear"].add(quad.shear_mean, quad.num);
            }
            if (quad.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(quad.shape_mean, quad.num);
            }
            if (quad.distortion_mean != std::numeric_limits<double>::max()) {
                metrics["distortion"].add(quad.distortion_mean, quad.num);
            }
            if (quad.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(quad.equiangle_skew_mean, quad.num);
            }
            if (quad.relative_size_squared_mean != std::numeric_limits<double>::max()) {
                metrics["relative_size_squared"].add(quad.relative_size_squared_mean, quad.num);
            }
            if (quad.shape_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shape_and_size"].add(quad.shape_and_size_mean, quad.num);
            }
            if (quad.shear_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shear_and_size"].add(quad.shear_and_size_mean, quad.num);
            }
            if (quad.enhanced_orth_mean != std::numeric_limits<double>::max()) {
                metrics["enhanced_orth"].add(quad.enhanced_orth_mean, quad.num);
            }
            // ===== 四面体贡献 =====
            const auto& tet = qmm.tetMaxMin;
            if (tet.edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["edge_ratio"].add(tet.edge_ratio_mean, tet.num);
            }
            if (tet.radius_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["radius_ratio"].add(tet.radius_ratio_mean, tet.num);
            }
            if (tet.aspect_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_ratio"].add(tet.aspect_ratio_mean, tet.num);
            }
            if (tet.aspect_gamma_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_gamma"].add(tet.aspect_gamma_mean, tet.num);
            }
            if (tet.aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["aspect_frobenius"].add(tet.aspect_frobenius_mean, tet.num);
            }
            if (tet.minimum_angle_mean != std::numeric_limits<double>::max()) {
                metrics["minimum_angle"].add(tet.minimum_angle_mean, tet.num);
            }
            if (tet.collapse_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["collapse_ratio"].add(tet.collapse_ratio_mean, tet.num);
            }
            if (tet.volume_mean != std::numeric_limits<double>::max()) {
                metrics["volume"].add(tet.volume_mean, tet.num);
            }
            if (tet.condition_mean != std::numeric_limits<double>::max()) {
                metrics["condition"].add(tet.condition_mean, tet.num);
            }
            if (tet.jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["jacobian"].add(tet.jacobian_mean, tet.num);
            }
            if (tet.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(tet.scaled_jacobian_mean, tet.num);
            }
            if (tet.mean_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["mean_ratio"].add(tet.mean_ratio_mean, tet.num);
            }
            if (tet.normalized_inradius_mean != std::numeric_limits<double>::max()) {
                metrics["normalized_inradius"].add(tet.normalized_inradius_mean, tet.num);
            }
            if (tet.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(tet.shape_mean, tet.num);
            }
            if (tet.distortion_mean != std::numeric_limits<double>::max()) {
                metrics["distortion"].add(tet.distortion_mean, tet.num);
            }
            if (tet.equivolume_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equivolume_skew"].add(tet.equivolume_skew_mean, tet.num);
            }
            if (tet.squish_index_mean != std::numeric_limits<double>::max()) {
                metrics["squish_index"].add(tet.squish_index_mean, tet.num);
            }
            if (tet.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(tet.equiangle_skew_mean, tet.num);
            }
            if (tet.relative_size_squared_mean != std::numeric_limits<double>::max()) {
                metrics["relative_size_squared"].add(tet.relative_size_squared_mean, tet.num);
            }
            if (tet.shape_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shape_and_size"].add(tet.shape_and_size_mean, tet.num);
            }
            if (tet.enhanced_orth_mean != std::numeric_limits<double>::max()) {
                metrics["enhanced_orth"].add(tet.enhanced_orth_mean, tet.num);
            }
            // ===== 六面体贡献 =====
            const auto& hex = qmm.hexMaxMin;
            if (hex.edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["edge_ratio"].add(hex.edge_ratio_mean, hex.num);
            }
            if (hex.max_edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["max_edge_ratio"].add(hex.max_edge_ratio_mean, hex.num);
            }
            if (hex.skew_mean != std::numeric_limits<double>::max()) {
                metrics["skew"].add(hex.skew_mean, hex.num);
            }
            if (hex.taper_mean != std::numeric_limits<double>::max()) {
                metrics["taper"].add(hex.taper_mean, hex.num);
            }
            if (hex.volume_mean != std::numeric_limits<double>::max()) {
                metrics["volume"].add(hex.volume_mean, hex.num);
            }
            if (hex.stretch_mean != std::numeric_limits<double>::max()) {
                metrics["stretch"].add(hex.stretch_mean, hex.num);
            }
            if (hex.diagonal_mean != std::numeric_limits<double>::max()) {
                metrics["diagonal"].add(hex.diagonal_mean, hex.num);
            }
            if (hex.dimension_mean != std::numeric_limits<double>::max()) {
                metrics["dimension"].add(hex.dimension_mean, hex.num);
            }
            if (hex.oddy_mean != std::numeric_limits<double>::max()) {
                metrics["oddy"].add(hex.oddy_mean, hex.num);
            }
            if (hex.med_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["med_aspect_frobenius"].add(hex.med_aspect_frobenius_mean, hex.num);
            }
            if (hex.max_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["max_aspect_frobenius"].add(hex.max_aspect_frobenius_mean, hex.num);
            }
            if (hex.condition_mean != std::numeric_limits<double>::max()) {
                metrics["condition"].add(hex.condition_mean, hex.num);
            }
            if (hex.jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["jacobian"].add(hex.jacobian_mean, hex.num);
            }
            if (hex.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(hex.scaled_jacobian_mean, hex.num);
            }
            if (hex.shear_mean != std::numeric_limits<double>::max()) {
                metrics["shear"].add(hex.shear_mean, hex.num);
            }
            if (hex.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(hex.shape_mean, hex.num);
            }
            if (hex.distortion_mean != std::numeric_limits<double>::max()) {
                metrics["distortion"].add(hex.distortion_mean, hex.num);
            }
            if (hex.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(hex.equiangle_skew_mean, hex.num);
            }
            if (hex.nodal_jacobian_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["nodal_jacobian_ratio"].add(hex.nodal_jacobian_ratio_mean, hex.num);
            }
            if (hex.relative_size_squared_mean != std::numeric_limits<double>::max()) {
                metrics["relative_size_squared"].add(hex.relative_size_squared_mean, hex.num);
            }
            if (hex.shape_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shape_and_size"].add(hex.shape_and_size_mean, hex.num);
            }
            if (hex.shear_and_size_mean != std::numeric_limits<double>::max()) {
                metrics["shear_and_size"].add(hex.shear_and_size_mean, hex.num);
            }
            if (hex.enhanced_orth_mean != std::numeric_limits<double>::max()) {
                metrics["enhanced_orth"].add(hex.enhanced_orth_mean, hex.num);
            }
            // ===== 楔形(五面体)贡献 =====
            const auto& wedge = qmm.wedgeMaxMin;
            if (wedge.volume_mean != std::numeric_limits<double>::max()) {
                metrics["volume"].add(wedge.volume_mean, wedge.num);
            }
            if (wedge.edge_ratio_mean != std::numeric_limits<double>::max()) {
                metrics["edge_ratio"].add(wedge.edge_ratio_mean, wedge.num);
            }
            if (wedge.max_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["max_aspect_frobenius"].add(wedge.max_aspect_frobenius_mean, wedge.num);
            }
            if (wedge.mean_aspect_frobenius_mean != std::numeric_limits<double>::max()) {
                metrics["mean_aspect_frobenius"].add(wedge.mean_aspect_frobenius_mean, wedge.num);
            }
            if (wedge.jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["jacobian"].add(wedge.jacobian_mean, wedge.num);
            }
            if (wedge.distortion_mean != std::numeric_limits<double>::max()) {
                metrics["distortion"].add(wedge.distortion_mean, wedge.num);
            }
            if (wedge.max_stretch_mean != std::numeric_limits<double>::max()) {
                metrics["max_stretch"].add(wedge.max_stretch_mean, wedge.num);
            }
            if (wedge.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(wedge.scaled_jacobian_mean, wedge.num);
            }
            if (wedge.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(wedge.shape_mean, wedge.num);
            }
            if (wedge.condition_mean != std::numeric_limits<double>::max()) {
                metrics["condition"].add(wedge.condition_mean, wedge.num);
            }
            if (wedge.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(wedge.equiangle_skew_mean, wedge.num);
            }
            if (wedge.enhanced_orth_mean != std::numeric_limits<double>::max()) {
                metrics["enhanced_orth"].add(wedge.enhanced_orth_mean, wedge.num);
            }
            // ===== 楔形(五面体)贡献 =====
            const auto& pyramid = qmm.pyramidMaxMin;
            if (pyramid.volume_mean != std::numeric_limits<double>::max()) {
                metrics["volume"].add(pyramid.volume_mean, pyramid.num);
            }
            if (pyramid.jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["jacobian"].add(pyramid.jacobian_mean, pyramid.num);
            }
            if (pyramid.scaled_jacobian_mean != std::numeric_limits<double>::max()) {
                metrics["scaled_jacobian"].add(pyramid.scaled_jacobian_mean, pyramid.num);
            }
            if (pyramid.shape_mean != std::numeric_limits<double>::max()) {
                metrics["shape"].add(pyramid.shape_mean, pyramid.num);
            }
            if (pyramid.equiangle_skew_mean != std::numeric_limits<double>::max()) {
                metrics["equiangle_skew"].add(pyramid.equiangle_skew_mean, pyramid.num);
            }

            // 计算所有38个平均值并赋值给QualityMaxMin
            qmm.length_mean = metrics["length"].getAverage();
            qmm.edge_ratio_mean = metrics["edge_ratio"].getAverage();
            qmm.max_edge_ratio_mean = metrics["max_edge_ratio"].getAverage();
            qmm.aspect_ratio_mean = metrics["aspect_ratio"].getAverage();
            qmm.radius_ratio_mean = metrics["radius_ratio"].getAverage();
            qmm.aspect_frobenius_mean = metrics["aspect_frobenius"].getAverage();
            qmm.med_aspect_frobenius_mean = metrics["med_aspect_frobenius"].getAverage();
            qmm.max_aspect_frobenius_mean = metrics["max_aspect_frobenius"].getAverage();
            qmm.skew_mean = metrics["skew"].getAverage();
            qmm.taper_mean = metrics["taper"].getAverage();
            qmm.warpage_mean = metrics["warpage"].getAverage();
            qmm.area_mean = metrics["area"].getAverage();
            qmm.stretch_mean = metrics["stretch"].getAverage();
            qmm.minimum_angle_mean = metrics["minimum_angle"].getAverage();
            qmm.maximum_angle_mean = metrics["maximum_angle"].getAverage();
            qmm.oddy_mean = metrics["oddy"].getAverage();
            qmm.condition_mean = metrics["condition"].getAverage();
            qmm.jacobian_mean = metrics["jacobian"].getAverage();
            qmm.scaled_jacobian_mean = metrics["scaled_jacobian"].getAverage();
            qmm.shear_mean = metrics["shear"].getAverage();
            qmm.shape_mean = metrics["shape"].getAverage();
            qmm.distortion_mean = metrics["distortion"].getAverage();
            qmm.equiangle_skew_mean = metrics["equiangle_skew"].getAverage();
            qmm.normalized_inradius_mean = metrics["normalized_inradius"].getAverage();
            qmm.relative_size_squared_mean = metrics["relative_size_squared"].getAverage();
            qmm.shape_and_size_mean = metrics["shape_and_size"].getAverage();
            qmm.shear_and_size_mean = metrics["shear_and_size"].getAverage();
            qmm.aspect_gamma_mean = metrics["aspect_gamma"].getAverage();
            qmm.collapse_ratio_mean = metrics["collapse_ratio"].getAverage();
            qmm.volume_mean = metrics["volume"].getAverage();
            qmm.mean_ratio_mean = metrics["mean_ratio"].getAverage();
            qmm.equivolume_skew_mean = metrics["equivolume_skew"].getAverage();
            qmm.squish_index_mean = metrics["squish_index"].getAverage();
            qmm.diagonal_mean = metrics["diagonal"].getAverage();
            qmm.dimension_mean = metrics["dimension"].getAverage();
            qmm.nodal_jacobian_ratio_mean = metrics["nodal_jacobian_ratio"].getAverage();
            qmm.max_stretch_mean = metrics["max_stretch"].getAverage();
            qmm.mean_aspect_frobenius_mean = metrics["mean_aspect_frobenius"].getAverage();
            qmm.enhanced_orth_mean = metrics["enhanced_orth"].getAverage();
        }
        //处理线网格
        void aggregate(const QualityMaxMin::LineMaxMin& line) {
            updateMinMax(qmm.length, line.length);
        }
        // 处理三角形
        void aggregate(const QualityMaxMin::TriMaxMin& tri) {
            updateMinMax(qmm.edge_ratio, tri.edge_ratio);
            updateMinMax(qmm.aspect_ratio, tri.aspect_ratio);
            updateMinMax(qmm.radius_ratio, tri.radius_ratio);
            updateMinMax(qmm.aspect_frobenius, tri.aspect_frobenius);
            updateMinMax(qmm.area, tri.area);
            updateMinMax(qmm.minimum_angle, tri.minimum_angle);
            updateMinMax(qmm.maximum_angle, tri.maximum_angle);
            updateMinMax(qmm.condition, tri.condition);
            updateMinMax(qmm.scaled_jacobian, tri.scaled_jacobian);
            updateMinMax(qmm.shape, tri.shape);
            updateMinMax(qmm.distortion, tri.distortion);
            updateMinMax(qmm.equiangle_skew, tri.equiangle_skew);
            updateMinMax(qmm.normalized_inradius, tri.normalized_inradius);
            updateMinMax(qmm.relative_size_squared, tri.relative_size_squared);
            updateMinMax(qmm.shape_and_size, tri.shape_and_size);
            updateMinMax(qmm.enhanced_orth, tri.enhanced_orth);
        }
        // 处理四边形
        void aggregate(const QualityMaxMin::QuadMaxMin& quad) {
            updateMinMax(qmm.edge_ratio, quad.edge_ratio);
            updateMinMax(qmm.max_edge_ratio, quad.max_edge_ratio);
            updateMinMax(qmm.aspect_ratio, quad.aspect_ratio);
            updateMinMax(qmm.radius_ratio, quad.radius_ratio);
            updateMinMax(qmm.med_aspect_frobenius, quad.med_aspect_frobenius);
            updateMinMax(qmm.max_aspect_frobenius, quad.max_aspect_frobenius);
            updateMinMax(qmm.skew, quad.skew);
            updateMinMax(qmm.taper, quad.taper);
            updateMinMax(qmm.warpage, quad.warpage);
            updateMinMax(qmm.area, quad.area);
            updateMinMax(qmm.stretch, quad.stretch);
            updateMinMax(qmm.minimum_angle, quad.minimum_angle);
            updateMinMax(qmm.maximum_angle, quad.maximum_angle);
            updateMinMax(qmm.oddy, quad.oddy);
            updateMinMax(qmm.condition, quad.condition);
            updateMinMax(qmm.jacobian, quad.jacobian);
            updateMinMax(qmm.scaled_jacobian, quad.scaled_jacobian);
            updateMinMax(qmm.shear, quad.shear);
            updateMinMax(qmm.shape, quad.shape);
            updateMinMax(qmm.distortion, quad.distortion);
            updateMinMax(qmm.equiangle_skew, quad.equiangle_skew);
            updateMinMax(qmm.relative_size_squared, quad.relative_size_squared);
            updateMinMax(qmm.shape_and_size, quad.shape_and_size);
            updateMinMax(qmm.shear_and_size, quad.shear_and_size);
            updateMinMax(qmm.enhanced_orth, quad.enhanced_orth);
        }
        // 处理四面体
        void aggregate(const QualityMaxMin::TetMaxMin& tet) {
            updateMinMax(qmm.edge_ratio, tet.edge_ratio);
            updateMinMax(qmm.radius_ratio, tet.radius_ratio);
            updateMinMax(qmm.aspect_ratio, tet.aspect_ratio);
            updateMinMax(qmm.aspect_gamma, tet.aspect_gamma);
            updateMinMax(qmm.aspect_frobenius, tet.aspect_frobenius);
            updateMinMax(qmm.minimum_angle, tet.minimum_angle);
            updateMinMax(qmm.collapse_ratio, tet.collapse_ratio);
            updateMinMax(qmm.volume, tet.volume);
            updateMinMax(qmm.condition, tet.condition);
            updateMinMax(qmm.jacobian, tet.jacobian);
            updateMinMax(qmm.scaled_jacobian, tet.scaled_jacobian);
            updateMinMax(qmm.mean_ratio, tet.mean_ratio);
            updateMinMax(qmm.normalized_inradius, tet.normalized_inradius);
            updateMinMax(qmm.shape, tet.shape);
            updateMinMax(qmm.distortion, tet.distortion);
            updateMinMax(qmm.equivolume_skew, tet.equivolume_skew);
            updateMinMax(qmm.squish_index, tet.squish_index);
            updateMinMax(qmm.equiangle_skew, tet.equiangle_skew);
            updateMinMax(qmm.relative_size_squared, tet.relative_size_squared);
            updateMinMax(qmm.shape_and_size, tet.shape_and_size);
            updateMinMax(qmm.enhanced_orth, tet.enhanced_orth);
        }
        // 处理六面体
        void aggregate(const QualityMaxMin::HexMaxMin& hex) {
            updateMinMax(qmm.edge_ratio, hex.edge_ratio);
            updateMinMax(qmm.max_edge_ratio, hex.max_edge_ratio);
            updateMinMax(qmm.skew, hex.skew);
            updateMinMax(qmm.taper, hex.taper);
            updateMinMax(qmm.volume, hex.volume);
            updateMinMax(qmm.stretch, hex.stretch);
            updateMinMax(qmm.diagonal, hex.diagonal);
            updateMinMax(qmm.dimension, hex.dimension);
            updateMinMax(qmm.oddy, hex.oddy);
            updateMinMax(qmm.med_aspect_frobenius, hex.med_aspect_frobenius);
            updateMinMax(qmm.max_aspect_frobenius, hex.max_aspect_frobenius);
            updateMinMax(qmm.condition, hex.condition);
            updateMinMax(qmm.jacobian, hex.jacobian);
            updateMinMax(qmm.scaled_jacobian, hex.scaled_jacobian);
            updateMinMax(qmm.shear, hex.shear);
            updateMinMax(qmm.shape, hex.shape);
            updateMinMax(qmm.distortion, hex.distortion);
            updateMinMax(qmm.equiangle_skew, hex.equiangle_skew);
            updateMinMax(qmm.nodal_jacobian_ratio, hex.nodal_jacobian_ratio);
            updateMinMax(qmm.relative_size_squared, hex.relative_size_squared);
            updateMinMax(qmm.shape_and_size, hex.shape_and_size);
            updateMinMax(qmm.shear_and_size, hex.shear_and_size);
            updateMinMax(qmm.enhanced_orth, hex.enhanced_orth);
        }
        // 处理楔形单元
        void aggregate(const QualityMaxMin::WedgeMaxMin& wedge) {
            updateMinMax(qmm.volume, wedge.volume);
            updateMinMax(qmm.edge_ratio, wedge.edge_ratio);
            updateMinMax(qmm.max_aspect_frobenius, wedge.max_aspect_frobenius);
            updateMinMax(qmm.mean_aspect_frobenius, wedge.mean_aspect_frobenius);
            updateMinMax(qmm.jacobian, wedge.jacobian);
            updateMinMax(qmm.scaled_jacobian, wedge.scaled_jacobian);
            updateMinMax(qmm.shape, wedge.shape);
            updateMinMax(qmm.condition, wedge.condition);
            updateMinMax(qmm.distortion, wedge.distortion);
            updateMinMax(qmm.equiangle_skew, wedge.equiangle_skew);
            updateMinMax(qmm.max_stretch, wedge.max_stretch);
            updateMinMax(qmm.enhanced_orth, wedge.enhanced_orth);
        }
        // 处理金字塔单元
        void aggregate(const QualityMaxMin::PyramidMaxMin& pyramid) {
            updateMinMax(qmm.volume, pyramid.volume);
            updateMinMax(qmm.jacobian, pyramid.jacobian);
            updateMinMax(qmm.scaled_jacobian, pyramid.scaled_jacobian);
            updateMinMax(qmm.shape, pyramid.shape);
            updateMinMax(qmm.equiangle_skew, pyramid.equiangle_skew);
        }
        QualityAggregator(QualityMaxMin& qmm_ref) : qmm(qmm_ref) {}
        // 主聚合函数
        template<typename T>
        void aggregate(const T& obj) {
            aggregate(obj); // 调用重载的特定类型函数
        }

        // 批量聚合
        template<typename Container>
        void aggregateAll(const Container& container) {
            for (const auto& item : container) {
                aggregate(item);
            }
        }
    };

	//网格面板
	class MMAINWINDOW_EXPORT mQualityPanel :public mMeshPanel
	{
	public:

        mQualityPanel(QWidget* parent = Q_NULLPTR);

		~mQualityPanel();

        //全部禁用
        void setDisableAll();
        //初始化指标
        void initMetrics();
        //应用检查
        void analysis();
        //一键全选
        void chooseAllCheckBox();
        //清空选择
        void clearAllCheckBox();

		//更新面板
		void editMeshPanel(int meshId);
        //禁用/启用指定行
        void setRowEnabled(QTableWidget* table, int row, bool enabled);
        //按可见顺序重排表格垂直表头序号：可见行连续编号，隐藏行留空
        void renumberVisibleRows(QTableWidget* table);
        //根据网格类型启用指定指标
        void setTableEnable();
        //计算最大最小、平均值
        void setTableMaxMin();

        void setWedge(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setHex(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setTet(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setQuad(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setTri(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setLine(MechMainWindow::QualityMaxMin& qualityMaxMin);

        void setPyramid(MechMainWindow::QualityMaxMin& qualityMaxMin);

	private:
		//全局尺寸

        QualityCheckContext* ctx;
		vector<RenderPickResultData> MeshIDs;
        QComboBox* shapeBox;
        QComboBox* standardBox;

        QCheckBox* line;
        QCheckBox* tri;
        QCheckBox* quad;
        QCheckBox* tet;
        QCheckBox* hex;
        QCheckBox* wedge;
        QCheckBox* pyramid;

        std::vector<QString> sizeMap =
        { {"边长比"},{"长度"},{"纵横比"}, {"Frobenius纵横比"},{"半径比"},
            {"面积"},{"归一化内切圆半径"},{"相对尺寸平方"},{"最大边长比"},{"中值Frobenius纵横比"},
            {"最大Frobenius纵横比"},{"纵横比Gamma"},{"体积"},{"对角线"},{"维度"},{"平均Frobenius纵横比"}
        };
        std::vector<QString> shapeMap =
        { {"最小角"},{"最大角"},{"形状"},{"等角偏斜"},{"形状和尺寸"},
            {"偏斜度"},{"锥度"},{"翘曲度"},{"拉伸度"},{"剪切度"},
            {"剪切和尺寸"},{"坍塌比"},{"等体积偏斜"},{"挤压指数"},{"最大拉伸"},{"平均比"}
        };
        std::vector<QString> paramMap =
        { {"条件数"},{"缩放雅可比"},{"畸变"},{"雅可比"},{"Oddy指标"},
            {"节点雅可比比"},{"增强型正交质量"}
        };
        std::vector<QString> Metric =
        {
            {"边长比"},{"长度"},{"纵横比"}, {"Frobenius纵横比"},{"半径比"},
            {"面积"},{"归一化内切圆半径"},{"相对尺寸平方"},{"最大边长比"},{"中值Frobenius纵横比"},
            {"最大Frobenius纵横比"},{"纵横比Gamma"},{"体积"},{"对角线"},{"维度"},{"平均Frobenius纵横比"},
            {"最小角"},{"最大角"},{"形状"},{"等角偏斜"},{"形状和尺寸"},
            {"偏斜度"},{"锥度"},{"翘曲度"},{"拉伸度"},{"剪切度"},
            {"剪切和尺寸"},{"坍塌比"},{"等体积偏斜"},{"挤压指数"},{"最大拉伸"},{"平均比"},
            {"条件数"},{"缩放雅可比"},{"畸变"},{"雅可比"},{"Oddy指标"},
            {"节点雅可比比"},{"增强型正交质量"}
        };
        std::map<QString, MetricMinMax> MetricMap;
        QTableWidget* sizeTable;
        QTableWidget* shapeTable;
        QTableWidget* paramTable;
	};
}