#include "ColorBarWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QColor>
#include <QVector3D>

#include "mPostOneFrameRendData.h"
#include "mGlobalSignals.h"
#include "mCommonFunctions.h"   // unify-addobject-arch：色表 Colors/Values 段经 ColorLegend 台账下发
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mAppConfiguration.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mOtherFunction.h"

using namespace PostData;

namespace PostWindow
{
	ColorBarWidget::ColorBarWidget(QWidget *parent)
		: QWidget(parent), colorIndex_(-1), valueIndex_(-1), edit_(nullptr)
	{
		setMouseTracking(true);
	}

    void ColorBarWidget::getColorAndValue()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postModelDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postModelDB == nullptr)
            return;

		auto* dataProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
		if (dataProxy == nullptr)
			return;

        //获取颜色条阶数
		colorBarNum_ = postRender->getColorTableWindow()->GetColorCount();

        //获取颜色条对应颜色
        colors_.clear();
        unsigned char* colorTable = postRender->getColorTableWindow()->GetColorData();
        for (int i = colorBarNum_ * 3 - 1 ;i > 0; i = i - 3)
        {
            //填充自定义颜色条颜色数据
            QColor color;
            color.setRed(colorTable[i-2]);
            color.setGreen(colorTable[i-1]);
            color.setBlue(colorTable[i]);
            colors_.push_back(color);
        }

        //获取颜色条对应数值表
        values_.clear();
        std::vector<float> valueTable = postRender->getColorTableWindow()->GetValue();
        for (int i = 0 ; i < valueTable.size(); i++)
        {
            QString value = MxFunctions::mNumberToString(valueTable.at(i), 'e', 3);
            values_.push_back(value);
        }
    }

    void ColorBarWidget::getDefaultColorAndValue(QVector<QColor> colors, QVector<QString> values)
    {
        defaultColors_ = colors;
        defaultValues_ = values;
    }

    int ColorBarWidget::getDefaultColorNum()
    {
        return defaultColors_.size();
    }

    void ColorBarWidget::setEditlineHide()
	{
		if (edit_)
		{
			edit_->hide();
		}
	}

	void ColorBarWidget::setDefaultColorBar()
	{
        colorBarNum_ = defaultColors_.size();
		colors_ = defaultColors_;
		values_ = defaultValues_;
		update();

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr)  return;

        //设置默认颜色
        // 色表收编（unify-addobject-arch）：恢复默认色带/数值=ColorLegend resetColors/resetValues 段
        //（旧 setPostColor()/setPostValue() 原体，顺序按现状）
        {
            mxr::ColorLegendParams _lg;
            _lg.updateMask = mxr::ColorLegendParams::Mask_Colors | mxr::ColorLegendParams::Mask_Values;
            _lg.resetColors = true;
            _lg.resetValues = true;
            PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
        }

        //设置默认数值（并入上一口，段序按现状）

        postRend->updatePaint();
	}

    void ColorBarWidget::paintEvent(QPaintEvent *event)
	{
        if (colors_.size() == 0 || values_.size() == 0) return;
		QPainter painter(this);
		int barWidth = width() / 4;
		int barHeight = height() / (colorBarNum_ + 3);
		int lineHeight = 1;
		int top = 0;
		for (int i = 0; i < colorBarNum_; ++i) {
			top = i * (barHeight + lineHeight) + barHeight;

			painter.setBrush(colors_[i]);
			painter.setPen(Qt::NoPen);
			painter.drawRect(0, top, barWidth, barHeight);

			painter.setPen(QPen(Qt::black, lineHeight));
			painter.drawLine(0, top, barWidth * 1.5, top);

			//QRect numberArea = numberRect(i);
			//QFont font("Arial", 10);
			////font.setWeight(QFont::Bold);
			//painter.setFont(font);
			//painter.drawText(numberArea, Qt::AlignLeft, values_[i]);
		}

        for (int i = 0; i < colorBarNum_ + 1; i++)
        {
            QRect numberArea = numberRect(i);
            QFont font("Arial", 10);
            //font.setWeight(QFont::Bold);
            painter.setFont(font);
            painter.drawText(numberArea, Qt::AlignLeft, values_[i]);
        }

		top = colorBarNum_ * (barHeight + lineHeight) + barHeight;
		painter.setPen(QPen(Qt::black, lineHeight));
		painter.drawLine(0, top, barWidth * 1.5, top);
	}

	void ColorBarWidget::mouseMoveEvent(QMouseEvent *event)
	{
		updateHoverIndex(event->pos());
	}

	void ColorBarWidget::mousePressEvent(QMouseEvent *event)
	{
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr)  return;

		if (colorIndex_ >= 0 && colorIndex_ < colors_.size())
		{
			QColor newColor;
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				newColor = QColorDialog::getColor(colors_[colorIndex_], this, "选择颜色");
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				newColor = QColorDialog::getColor(colors_[colorIndex_], this, "Select Color");
			}

			if (newColor.isValid()) {
				colors_[colorIndex_] = newColor;
                update();

                //设置颜色条颜色
                float r = newColor.red();
                float g = newColor.green();
                float b = newColor.blue();
                QVector3D color(r, g, b);
                int index = colors_.size() - colorIndex_ - 1;
                Q_UNUSED(index);
                // 色表收编（unify-addobject-arch）：单档颜色=Colors 段整表下发
                //（本表按门面档序组表=widget 序反序，与旧 setPostColor(index,color)
                // 的反序映射逐项一致；SetColor 逐档幂等，未改档原值重设零视觉变化）
                {
                    mxr::ColorLegendParams _lg;
                    _lg.updateMask = mxr::ColorLegendParams::Mask_Colors;
                    for (int k = 0; k < colors_.size(); ++k)
                    {
                        const QColor& c = colors_[colors_.size() - 1 - k];   // 门面档序 k ← widget 序反序
                        _lg.colors.append(QVector3D(float(c.red()), float(c.green()), float(c.blue())));
                    }
                    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                }
                postRend->updatePaint();
			}
		}
		else if (valueIndex_ >= 0 && valueIndex_ < values_.size())
		{
			QRect numberArea = numberRect(valueIndex_);
			if (numberArea.contains(event->pos())) {
				if (edit_) {
					edit_->hide();
					edit_->deleteLater();
				}
				edit_ = new QLineEdit(this);
				edit_->setText(values_[valueIndex_]);
				int editIndex = valueIndex_;
				edit_->setFixedSize(80, 20);
				edit_->setGeometry(numberArea);
				edit_->show();
				edit_->setFocus();
				connect(edit_, &QLineEdit::returnPressed, [this, editIndex, postRender, postRend]() {
					if (edit_) {
						values_[editIndex] = edit_->text();
                        float value = edit_->text().toFloat();
                        Q_UNUSED(value);
                        // 色表收编（unify-addobject-arch）：单档数值=Values 段整表下发
                        //（本表按门面档序组表=widget 序直用，与旧 setPostValue(editIndex,value)
                        // 的直用映射逐项一致；SetValue 逐档幂等，未改档原值重设零视觉变化）
                        {
                            mxr::ColorLegendParams _lg;
                            _lg.updateMask = mxr::ColorLegendParams::Mask_Values;
                            for (int k = 0; k < values_.size(); ++k)
                                _lg.values.append(values_.at(k).toFloat());
                            PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                        }
                        postRend->updatePaint();
                        //获取颜色条数值
                        getColorAndValue();
                        //隐藏输入框
                        edit_->hide();
                        edit_->deleteLater();
                        edit_ = nullptr;
                        update();
					}
				});
			}

		}
		if (valueIndex_ == -1 || valueIndex_ > colors_.size())
		{
			if (edit_)
			{
				edit_->hide();
			}
		}
	}

    void ColorBarWidget::slot_finishedImportFiles(const QString& postname)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

		auto* dataProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
		if (dataProxy == nullptr)
			return;

        //获取颜色条对应颜色
        defaultColors_.clear();
        unsigned char* colorTable = postRender->getColorTableWindow()->GetColorData();
        for (int i = 35; i > 0; i = i - 3)
        {
            //填充自定义颜色条颜色数据
            QColor color;
            color.setRed(colorTable[i - 2]);
            color.setGreen(colorTable[i - 1]);
            color.setBlue(colorTable[i]);
            defaultColors_.push_back(color);
        }

        //获取颜色条对应数值表
        defaultValues_.clear();
        std::vector<float> valueTable = postRender->getColorTableWindow()->GetValue();
        for (int i = 0; i < valueTable.size(); i++)
        {
            QString value = MxFunctions::mNumberToString(valueTable.at(i), 'e', 3);
            defaultValues_.push_back(value);
        }
    }

    void ColorBarWidget::updateHoverIndex(const QPoint &pos)
	{
		int barHeight = height() / (colorBarNum_ + 3);
		int barWidth = width() / 4;
		int lineHeight = 1;
		if (pos.x() < barWidth  && pos.y() > barHeight)
		{
			int newIndex = (pos.y() - barHeight) / (barHeight + lineHeight);
			if (newIndex != colorIndex_)
			{
				colorIndex_ = newIndex;
				update();
			}
		}
		else if ((barWidth * 1.5 + 5) < pos.x() && pos.x() < (barWidth * 1.5 + 5 + 80))
		{
			int newIndex = (pos.y() - barHeight / 2) / (barHeight + lineHeight);
			if (newIndex != valueIndex_)
			{
				valueIndex_ = newIndex;
				update();
			}
		}
		if (pos.y() < barHeight / 2 ||
			pos.x() > (barWidth * 1.5 + 5 + 80) ||
			pos.x() < (barWidth * 1.5 + 5))
		{
			valueIndex_ = -1;
		}
		if (pos.y() < barHeight ||
			pos.x() > barWidth)
		{
			colorIndex_ = -1;
		}
	}

	QRect ColorBarWidget::numberRect(int index)
	{
		int barHeight = height() / (colorBarNum_ + 3);
		int lineHeight = 1;
		int barWidth = width() / 4;
		int top = index * (barHeight + lineHeight) + barHeight / 2;
		return QRect(barWidth * 1.5 + 5, top, 70, 20);
	}
}
