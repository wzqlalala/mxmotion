#include "mGeoRemovalPanel.h"
#include "ViewportContainer.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"


#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include <QMenu>
#include <QTimer>
#include <QTime>
#include <qsignalmapper.h>
#include "PickCommonFunction.h"
#include <mMeshData.h>

using namespace mxr;
namespace MechMainWindow
{
	mGeoRemovalPanel::mGeoRemovalPanel(QWidget* parent)
		: mGeoPanelBase(parent)
	{
		connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());

			if (geoPickWidget->pickObjTypeCombo->currentText() == "几何面")
			{
				if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
				{
					clearTableItems(pickObjTable);
				}
			}
			else if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
			{
				if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何面"))
				{
					clearTableItems(pickObjTable);
				}
			}
			
			geoPickWidget->_featPick = FeatureObj;
			});

		initRemovePanel();
		_mainLayout->addStretch();

        //全局信号（拾取/单位制/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接

        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxHoleRadiusUnit, maxHoleRadius));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxSolidHoleRadiusUnit, maxSolidHoleRadius));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxFilletRadiusUnit, maxFilletRadius));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxChamferDisUnit, maxChamferDis));
        //m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxMinorDiameterUnit, maxMinorDiameter));
	}

	void mGeoRemovalPanel::connectGeoSignals()
	{
		//拾取信号
		slot_subWindowActivated();

		// 新架构：直接挂容器激活信号（带 prevID，可区分真正的视口切换）
		if (auto* container = MechPreData::mViewportModelManager::getInstance()->getViewportContainer()) {
			conn_viewportChanged = connect(container, &MechMainWindow::ViewportContainer::sig_activeViewportChanged,
				this, [this](int viewportID, int prevID) {
					Q_UNUSED(viewportID); Q_UNUSED(prevID);
					slot_subWindowActivated();
				});
		} else {
			conn_viewportChanged = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, [this](int) { slot_subWindowActivated(); });
		}

		//单位制信号
		connectUnitSystemResponse();
	}

	void mGeoRemovalPanel::disconnectGeoSignals()
	{
		if (conn_geo)
		{
			disconnect(conn_geo);
			conn_geo = QMetaObject::Connection();
		}
		if (conn_viewportChanged)
		{
			disconnect(conn_viewportChanged);
			conn_viewportChanged = QMetaObject::Connection();
		}
		disconnectUnitSystemResponse();
	}

	mGeoRemovalPanel::~mGeoRemovalPanel()
	{

	}


	void mGeoRemovalPanel::showFold(GeoOperationType type)
	{
		// 进入编辑时连接单位制切换信号（退出编辑由mInfoWindow::uneditGeometry统一断开）
		connectUnitSystemResponse();

		hideAllFold();
		
		//geoPickWidget->pickObjTypeCombo->clear();

		if (GeoAutoRemoval == type)
		{
			displayAfterInsertFolds({ 0 });
		}
		//if (type.compare("Manual") == 0)
		//{
		//	geoPickWidget->pickObjTypeCombo->addItem("几何面");
		//	displayAfterInsertFolds({ 1 });
		//}
		geoPickWidget->_featPick = FeatureObj;
		geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
	}

    void mGeoRemovalPanel::initRemovePanel()
    {
        //--自动去除
        autoRemoveHole1 = new QCheckBox("面孔 最大半径", this); autoRemoveHole1->setCheckState(Qt::Checked); autoRemoveHole1->setFixedWidth(120);
        autoRemoveHole2 = new QCheckBox("面孔 最大面积", this); autoRemoveHole2->setCheckState(Qt::Checked); autoRemoveHole2->setFixedWidth(120);
        autoRemoveSolidHole = new QCheckBox("实体孔 最大半径", this); autoRemoveSolidHole->setCheckState(Qt::Checked); autoRemoveSolidHole->setFixedWidth(120);
        autoRemoveFillet = new QCheckBox("倒圆角 最大半径", this); autoRemoveFillet->setCheckState(Qt::Checked); autoRemoveFillet->setFixedWidth(120);
        autoRemoveChamfer = new QCheckBox("倒斜角 最大长度", this); autoRemoveChamfer->setCheckState(Qt::Checked); autoRemoveChamfer->setFixedWidth(120);
        //autoRemoveThread = new QCheckBox("螺纹 最大内径",this); autoRemoveThread->setCheckState(Qt::Checked); autoRemoveThread->setFixedWidth(120);
        autoRemoveLogo = new QCheckBox("LOGO 最大高度", this); autoRemoveLogo->setCheckState(Qt::Checked); autoRemoveLogo->setFixedWidth(120);


        maxHoleRadius = new MxFunctions::mCalcLineEdit(this); maxHoleRadius->setExpression("10");
        maxHoleArea = new MxFunctions::mCalcLineEdit(this); maxHoleArea->setExpression("10");
        maxSolidHoleRadius = new MxFunctions::mCalcLineEdit(this); maxSolidHoleRadius->setExpression("10");
        maxFilletRadius = new MxFunctions::mCalcLineEdit(this); maxFilletRadius->setExpression("10");
        maxChamferDis = new MxFunctions::mCalcLineEdit(this); maxChamferDis->setExpression("10");
        //maxMinorDiameter = new MxFunctions::mCalcLineEdit(this); maxMinorDiameter->setExpression("10");
        maxLogoHeight = new MxFunctions::mCalcLineEdit(this); maxLogoHeight->setExpression("10");

        QPushButton* autoRemoveSearchBtn = new QPushButton("搜寻", this);
        QPushButton* autoRemoveCancelBtn = new QPushButton("取消", this);
        QPushButton* autoRemoveOkBtn = new QPushButton("确认", this);
        //autoRemoveOkBtn->setDisabled(true);

        connect(autoRemoveCancelBtn, &QPushButton::clicked, this, &mGeoRemovalPanel::slot_onCancel);
        connect(autoRemoveOkBtn, &QPushButton::clicked, this, &mGeoRemovalPanel::slot_autoRemoveAction);
        connect(autoRemoveSearchBtn, &QToolButton::clicked, this, [this]() {
            autoRemoveSearchAction();
            geoPickWidget->_featPick = FeatureObj;
            });

        connect(autoRemoveHole1, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveHole1->isChecked()) {
                maxHoleRadius->setDisabled(false);
                QString text = maxHoleRadius->placeholderText();
                maxHoleRadius->setFocus();
                maxHoleRadius->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveSolidHole->setCheckState(Qt::Unchecked);
                autoRemoveFillet->setCheckState(Qt::Unchecked);
                autoRemoveHole2->setCheckState(Qt::Unchecked);
                autoRemoveChamfer->setCheckState(Qt::Unchecked);
                //autoRemoveThread->setCheckState(Qt::Unchecked);
                autoRemoveLogo->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何线")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
                if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何面"))
                {
                    clearTableItems(pickObjTable);
                }
            }
            else {
                maxHoleRadius->setDisabled(true);
                QString text = maxHoleRadius->text();
                //maxHoleRadius->setFocus();
                maxHoleRadius->setText(0);
                maxHoleRadius->setPlaceholderText(text);
                if (!autoRemoveHole2->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }
                if (!autoRemoveFillet->checkState() && !autoRemoveHole2->checkState() &&
                    !autoRemoveSolidHole->checkState() && !autoRemoveChamfer->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }

            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何面"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            });

        connect(autoRemoveHole2, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveHole2->isChecked()) {
                maxHoleArea->setDisabled(false);
                QString text = maxHoleArea->placeholderText();
                maxHoleArea->setFocus();
                maxHoleArea->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveSolidHole->setCheckState(Qt::Unchecked);
                autoRemoveFillet->setCheckState(Qt::Unchecked);
                autoRemoveHole1->setCheckState(Qt::Unchecked);
                autoRemoveChamfer->setCheckState(Qt::Unchecked);
                //autoRemoveThread->setCheckState(Qt::Unchecked);
                autoRemoveLogo->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何线")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
            }
            else {
                maxHoleArea->setDisabled(true);
                QString text = maxHoleArea->text();
                maxHoleArea->clearFocus();
                //maxHoleArea->setText(0);
                maxHoleArea->setPlaceholderText(text);

                if (!autoRemoveHole1->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }

                if (!autoRemoveFillet->checkState() && !autoRemoveHole1->checkState() &&
                    !autoRemoveSolidHole->checkState() && !autoRemoveChamfer->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }
            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何面"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            });

        connect(autoRemoveSolidHole, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveSolidHole->isChecked()) {
                maxSolidHoleRadius->setDisabled(false);
                QString text = maxSolidHoleRadius->placeholderText();
                maxSolidHoleRadius->setFocus();
                maxSolidHoleRadius->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveHole1->setCheckState(Qt::Unchecked);
                autoRemoveHole2->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }
            }
            else {
                maxSolidHoleRadius->setDisabled(true);
                QString text = maxSolidHoleRadius->text();
                maxSolidHoleRadius->clearFocus();
                //maxSolidHoleRadius->setText(0);
                maxSolidHoleRadius->setPlaceholderText(text);
                if (!autoRemoveFillet->checkState() && !autoRemoveChamfer->checkState()/* &&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
                if (!autoRemoveFillet->checkState() && !autoRemoveHole1->checkState() &&
                    !autoRemoveHole2->checkState() && !autoRemoveChamfer->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }
            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            });



        connect(autoRemoveFillet, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveFillet->isChecked()) {
                maxFilletRadius->setDisabled(false);
                QString text = maxFilletRadius->placeholderText();
                maxFilletRadius->setFocus();
                maxFilletRadius->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveHole1->setCheckState(Qt::Unchecked);
                autoRemoveHole2->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }
                if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
                {
                    clearTableItems(pickObjTable);
                }
            }
            else {
                maxFilletRadius->setDisabled(true);
                QString text = maxFilletRadius->text();
                maxFilletRadius->clearFocus();
                //maxFilletRadius->setText(0);
                maxFilletRadius->setPlaceholderText(text);
                if (!autoRemoveSolidHole->checkState() && !autoRemoveChamfer->checkState()/* &&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
                if (!autoRemoveSolidHole->checkState() && !autoRemoveHole1->checkState() &&
                    !autoRemoveHole2->checkState() && !autoRemoveChamfer->checkState()/* &&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }
            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            });

        connect(autoRemoveChamfer, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveChamfer->isChecked()) {
                maxChamferDis->setDisabled(false);
                QString text = maxChamferDis->placeholderText();
                maxChamferDis->setFocus();
                maxChamferDis->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveHole1->setCheckState(Qt::Unchecked);
                autoRemoveHole2->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }
            }
            else {
                maxChamferDis->setDisabled(true);
                QString text = maxChamferDis->text();
                maxChamferDis->clearFocus();
                //maxChamferDis->setText(0);
                maxChamferDis->setPlaceholderText(text);
                if (!autoRemoveSolidHole->checkState() && !autoRemoveFillet->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
                if (!autoRemoveSolidHole->checkState() && !autoRemoveHole1->checkState() &&
                    !autoRemoveHole2->checkState() && !autoRemoveFillet->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveLogo->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }
            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            });

        //connect(autoRemoveThread, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
        //	if (autoRemoveThread->isChecked()) {
        //		maxMinorDiameter->setDisabled(false);
        //		QString text = maxMinorDiameter->placeholderText();
        //		maxMinorDiameter->setFocus();
        //		maxMinorDiameter->setText(text);
        //		autoRemoveSearchBtn->setDisabled(false);
        //		autoRemoveHole1->setCheckState(Qt::Unchecked);
        //		autoRemoveHole2->setCheckState(Qt::Unchecked);
        //		if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
        //		{
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItem("几何面");
        //			geoPickWidget->strPickType = "几何面";
        //		}
        //		if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
        //		{
        //			clearTableItems(pickObjTable);
        //		}
        //	}
        //	else {
        //		maxMinorDiameter->setDisabled(true);
        //		QString text = maxMinorDiameter->text();
        //		maxMinorDiameter->clearFocus();
        //		//maxMinorDiameter->setText(0);
        //		maxMinorDiameter->setPlaceholderText(text);
        //		if (!autoRemoveSolidHole->checkState() && !autoRemoveFillet->checkState() &&
        //			!autoRemoveChamfer->checkState() && !autoRemoveLogo->checkState())
        //		{
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItem("几何线");
        //			geoPickWidget->strPickType = "几何线";
        //		}
        //		if (!autoRemoveSolidHole->checkState() && !autoRemoveHole1->checkState() &&
        //			!autoRemoveHole2->checkState() && !autoRemoveChamfer->checkState() &&
        //			!autoRemoveFillet->checkState() && !autoRemoveLogo->checkState())
        //		{
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
        //			geoPickWidget->strPickType = "几何线";
        //			autoRemoveSearchBtn->setDisabled(true);
        //		}
        //	}
        //	if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
        //	{
        //		clearTableItems(pickObjTable);
        //	}
        //	CFDCommon::mCFDGlobalSignals::getInstance()->activePickSig(3);
        //	});

        connect(autoRemoveLogo, &QCheckBox::stateChanged, this, [this, autoRemoveSearchBtn]() {
            if (autoRemoveLogo->isChecked()) {
                maxLogoHeight->setDisabled(false);
                QString text = maxLogoHeight->placeholderText();
                maxLogoHeight->setFocus();
                maxLogoHeight->setText(text);
                autoRemoveSearchBtn->setDisabled(false);
                autoRemoveHole1->setCheckState(Qt::Unchecked);
                autoRemoveHole2->setCheckState(Qt::Unchecked);
                if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何面");
                    geoPickWidget->strPickType = "几何面";
                }
                if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
                {
                    clearTableItems(pickObjTable);
                }
            }
            else {
                maxLogoHeight->setDisabled(true);
                QString text = maxLogoHeight->text();
                maxLogoHeight->setFocus();
                //maxLogoHeight->setText(0);
                maxLogoHeight->setPlaceholderText(text);
                if (!autoRemoveSolidHole->checkState() && !autoRemoveFillet->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveChamfer->checkState())
                {
                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItem("几何线");
                    geoPickWidget->strPickType = "几何线";
                }
                if (!autoRemoveSolidHole->checkState() && !autoRemoveHole1->checkState() &&
                    !autoRemoveHole2->checkState() && !autoRemoveChamfer->checkState() /*&&
                    !autoRemoveThread->checkState()*/ && !autoRemoveFillet->checkState())
                {

                    geoPickWidget->pickObjTypeCombo->clear();
                    geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
                    geoPickWidget->strPickType = "几何线";
                    autoRemoveSearchBtn->setDisabled(true);
                }
            }
            if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
            {
                clearTableItems(pickObjTable);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            });
        //【集成回补】上为 dev 活跃的 logo 复选框联动（合并时被误注释）；其孤儿碎片已清理
        //	if (autoRemoveLogo->isChecked()) {
        //		maxLogoArea->setDisabled(false);
        //		QString text = maxLogoArea->placeholderText();
        //		maxLogoArea->setFocus();
        //		maxLogoArea->setText(text);
        //		autoRemoveSearchBtn->setDisabled(false);
        //		autoRemoveHole1->setCheckState(Qt::Unchecked);
        //		autoRemoveHole2->setCheckState(Qt::Unchecked);
        //		if (geoPickWidget->pickObjTypeCombo->currentText() != "几何面")
        //		{
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItem("几何面");
        //			geoPickWidget->strPickType = "几何面";
        //		}
        //		if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
        //		{
        //			clearTableItems(pickObjTable);
        //		}
        //	}
        //	else {
        //		maxLogoArea->setDisabled(true);
        //		QString text = maxLogoArea->text();
        //		maxLogoArea->setFocus();
        //		//maxLogoArea->setText(0);
        //		maxLogoArea->setPlaceholderText(text);
        //		if (!autoRemoveSolidHole->checkState() && !autoRemoveFillet->checkState() &&
        //			!autoRemoveThread->checkState() && !autoRemoveChamfer->checkState())
        //		{
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItem("几何线");
        //			geoPickWidget->strPickType = "几何线";
        //		}
        //		if (!autoRemoveSolidHole->checkState() && !autoRemoveHole1->checkState() &&
        //			!autoRemoveHole2->checkState() && !autoRemoveChamfer->checkState() &&
        //			!autoRemoveThread->checkState() && !autoRemoveFillet->checkState())
        //		{
        //			
        //			geoPickWidget->pickObjTypeCombo->clear();
        //			geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
        //			geoPickWidget->strPickType = "几何线";
        //			autoRemoveSearchBtn->setDisabled(true);
        //		}
        //	}
        //	if (pickObjTable->rowCount() > 0 && pickObjTable->item(0, 0)->text().contains("几何线"))
        //	{
        //		clearTableItems(pickObjTable);
        //	}
        //	CFDCommon::mCFDGlobalSignals::getInstance()->activePickSig(3);
        //	});

    
        maxHoleRadiusUnit = new QLabel("mm", this);
        maxSolidHoleRadiusUnit = new QLabel("mm", this);
        maxFilletRadiusUnit = new QLabel("mm", this);
        maxChamferDisUnit = new QLabel("mm", this);
        //maxMinorDiameterUnit = new QLabel("mm", this);

        QHBoxLayout* holeLayout = new QHBoxLayout(this);
        holeLayout->addWidget(autoRemoveHole1);
        holeLayout->addWidget(maxHoleRadius);
        holeLayout->addWidget(maxHoleRadiusUnit);

        QHBoxLayout* holeLayout2 = new QHBoxLayout(this);
        holeLayout2->addWidget(autoRemoveHole2);
        holeLayout2->addWidget(maxHoleArea);
        holeLayout2->addWidget(new QLabel("mm²"));

        QHBoxLayout* solidholeLayout = new QHBoxLayout(this);
        solidholeLayout->addWidget(autoRemoveSolidHole);
        solidholeLayout->addWidget(maxSolidHoleRadius);
        solidholeLayout->addWidget(maxSolidHoleRadiusUnit);

        QHBoxLayout* filletLayout = new QHBoxLayout(this);
        filletLayout->addWidget(autoRemoveFillet);
        filletLayout->addWidget(maxFilletRadius);
        filletLayout->addWidget(maxFilletRadiusUnit);

        QHBoxLayout* chamferLayout = new QHBoxLayout(this);
        chamferLayout->addWidget(autoRemoveChamfer);
        chamferLayout->addWidget(maxChamferDis);
        chamferLayout->addWidget(maxChamferDisUnit);

        /*QHBoxLayout* threadLayout = new QHBoxLayout(this);
        threadLayout->addWidget(autoRemoveThread);
        threadLayout->addWidget(maxMinorDiameter);
        threadLayout->addWidget(maxMinorDiameterUnit);*/

        QHBoxLayout* logoLayout = new QHBoxLayout(this);
        logoLayout->addWidget(autoRemoveLogo);
        logoLayout->addWidget(maxLogoHeight);
        logoLayout->addWidget(new QLabel("mm²"));


        FoldSection* autoRemoveFold = new FoldSection("自动去除", this);
        QFrame* hLine = createHorizontalLine(autoRemoveFold);
        autoRemoveFold->addRow(holeLayout);
        autoRemoveFold->addRow(holeLayout2);
        autoRemoveFold->addRow(solidholeLayout);
        autoRemoveFold->addRow(filletLayout);
        autoRemoveFold->addRow(chamferLayout);
        //autoRemoveFold->addRow(threadLayout);
        autoRemoveFold->addRow(logoLayout);

        FoldSection* RemoveAddFold = new FoldSection("手动去除", autoRemoveFold, QColor("#909399"), false);
        RemoveAddFold->addRow(geoPickWidget);
        RemoveAddFold->addRow(pickObjTable);
        autoRemoveFold->addRow(RemoveAddFold);
        //autoRemoveFold->addRow(new QLabel(" "), autoRemoveSearchBtn, "Left");
        autoRemoveFold->addRow(hLine);
        autoRemoveFold->addRow(autoRemoveSearchBtn, autoRemoveCancelBtn, autoRemoveOkBtn, "mid");

        connect(RemoveAddFold, &FoldWidget::foldExpandChanged, this, [autoRemoveFold, RemoveAddFold, this]() {
            autoRemoveFold->refreshLayout();
            });

		autoRemoveHole1->setCheckState(Qt::Checked);
		autoRemoveHole2->setCheckState(Qt::Unchecked);
        autoRemoveSolidHole->setCheckState(Qt::Unchecked);
		autoRemoveFillet->setCheckState(Qt::Unchecked);
		autoRemoveChamfer->setCheckState(Qt::Unchecked);
        //autoRemoveThread->setCheckState(Qt::Unchecked);
        autoRemoveLogo->setCheckState(Qt::Unchecked);
		autoRemoveSearchBtn->setDisabled(false);
		maxHoleRadius->setDisabled(false);
		maxHoleArea->setDisabled(true);
		maxSolidHoleRadius->setDisabled(true);
		maxFilletRadius->setDisabled(true);
		maxChamferDis->setDisabled(true);
		//maxMinorDiameter->setDisabled(true);//【GUI 门控】AUTOREMOVAL_V2 未推送，成员已注释
		//maxLogoArea->setDisabled(true);
		geoPickWidget->pickObjTypeCombo->clear();
		geoPickWidget->pickObjTypeCombo->addItem("几何线");
		geoPickWidget->strPickType = "几何线";

		//复选框勾选处理器从占位符回填输入框：先补齐占位符初值，
		//否则首次勾选会把 setExpression 预置的 "10" 清成空白（搜寻带 0 阈值必空）
		maxHoleRadius->setPlaceholderText("10");
		maxHoleArea->setPlaceholderText("10");
		maxSolidHoleRadius->setPlaceholderText("10");
		maxFilletRadius->setPlaceholderText("10");
		maxChamferDis->setPlaceholderText("10");
		maxLogoHeight->setPlaceholderText("10");

		m_FoldSections.append(autoRemoveFold);
		m_foldWidgets.append(autoRemoveFold);
	}
	

	void mGeoRemovalPanel::slot_highlightGeoData()
	{
		QString type = "几何线";

		int num = 0;
		vector<int> res;
		//获取拾取模式
		PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		switch (pickFilter)
		{
		case mxr::PickFilter::PickGeoSolid:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			type = "几何体";
			break;
		}
		case mxr::PickFilter::PickGeoFace:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			type = "几何面";
			break;
		}
		case mxr::PickFilter::PickGeoLine:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			type = "几何线";
			break;
		}
		case mxr::PickFilter::PickGeoPoint:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			type = "几何点";
			break;
		}
		default:
			break;
		}
		num = res.size();

		
		
		if (geoPickWidget->_featPick == FeatureObj)
		{
			for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
			{
				pickObjTable->removeRow(i);
			}

			if (geoPickWidget->strPickType == type)
			{
				for (auto id : res)
				{
					pickObjTable->insertRow(pickObjTable->rowCount());
					pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
				}
			}
		}
		if (geoPickWidget->_featPick == AutoRemoval)
		{
			if (autoRemovalPickFilter == pickFilter)
			{
				autoCollectIds.insert(res.begin(), res.end());
			}
			else
			{
				autoCollectIds.clear();
				autoCollectIds.insert(res.begin(), res.end());
			}

			autoRemovalPickFilter = pickFilter;
		}
		
	}

	void mGeoRemovalPanel::slot_subWindowActivated()
	{
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoRemovalPanel::slot_highlightGeoData);
	}

	void mGeoRemovalPanel::slot_onCancel()
	{
		hideAllFold();
	}

	void mGeoRemovalPanel::slot_autoRemoveAction()
	{
		try
		{
			set<int> manualIds = GetTableIdsUseSet();
			autoCollectIds.insert(manualIds.begin(),manualIds.end());

			if (autoCollectIds.empty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请先进行搜寻或者手动选择!"));
				return;
			}

			bool bRes = false;
			EventDispatcher::mGeoOperationResult res;

			if (geoPickWidget->pickObjTypeCombo->currentText() == "几何面")
			{
				if (autoRemoveSolidHole->isChecked())
				{
					// 实体孔重建仅 autoRemovalOCC 提供（引擎无等价编辑链）
					EventDispatcher::mGeoAutoRemovalRequest req;
					req.removalType = EventDispatcher::SolidHole;
					req.allSolidHoleFaceIds = autoCollectIds;
					res = getGEOServer.geoEdit().autoRemovalOCC(req);
				}
				else
				{
					// 移除特征面：与快捷键 F 同走 editOfDefeature（原走 autoRemovalOCC 的
					// Defeature 分支不上报空部件，确认后会残留空壳部件）
					EventDispatcher::mGeoDefeatureRequest req;
					req.geoIds.assign(autoCollectIds.begin(), autoCollectIds.end());
					res = getGEOServer.geoEdit().editOfDefeature(req);
				}
				bRes = res.succeeded();
				if (bRes)
				{
					for (auto editpart : res.modifiedObjectIds)
					{
						getGEOServer.context().PartManager()._editPart.insert(editpart.id);
					}
				}
			}
			else if(geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
			{
				EventDispatcher::mGeoAutoRemovalRequest req;
				req.removalType = EventDispatcher::FaceHole;
				req.edgeIds = autoCollectIds;
				if (autoRemoveHole1->isChecked())
				{
					req.holeMaxRadius = getDefaultValue(Unit::Length, maxHoleRadius->text().toDouble());
				}
				if (autoRemoveHole2->isChecked())
				{
					req.holeMaxArea = maxHoleArea->text().toDouble();
				}
				res = getGEOServer.geoEdit().autoRemovalOCC(req);
				bRes = res.succeeded();
				if (bRes)
				{
					for (auto editpart : res.modifiedObjectIds)
					{
						getGEOServer.context().PartManager()._editPart.insert(editpart.id);
					}
				}
			}

			if (bRes)
			{
				//特征移除后部件变空时联动删除应用侧镜像（同 F 键 geometryDeleteFeature 范式）
				if (res.removedObjectIds.size() > 0)
				{
					std::set<int> partIds;
					for (const auto& removed : res.removedObjectIds)
					{
						partIds.insert(removed.id);
					}
					getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
					getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
					MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
				}
				geoPickWidget->clearHighlight();
				autoCollectIds.clear();
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,tr("几何编辑成功！"));
				getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
			}
			else
			{
				//失败也清搜寻累计集：混有不可移除面的旧集合若不清，会继续污染
				//下一次确认（手动选择的干净面集被并集后整单失败）
				autoCollectIds.clear();
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败！"));
			}
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败！"));
		}
	}


	void mGeoRemovalPanel::slot_manualRemoveAction()
	{
		try
		{
			std::set<int> geoSolidIds = GetTableIdsUseSet();
			if (geoSolidIds.empty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何面!"), true);
				return;
			}

			// 手动移除特征面：走自动去除的 Defeature 分支
			EventDispatcher::mGeoAutoRemovalRequest req;
			req.removalType = EventDispatcher::Defeature;
			req.faceIds = geoSolidIds;

			EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().autoRemovalOCC(req);
			if (res.succeeded())
			{
				for (auto editpart : res.modifiedObjectIds)
				{
					getGEOServer.context().PartManager()._editPart.insert(editpart.id);
				}
				geoPickWidget->clearHighlight();
				clearTableItems(pickObjTable);
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,tr("几何编辑成功！"));
				getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败！"));
			}
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败！"));
		}

		return;
	}

	bool mGeoRemovalPanel::autoRemoveSearchAction()
	{
		try
		{
			QTime time;
			time.start();

			geoPickWidget->clearHighlight();

			// 构建识别请求
			EventDispatcher::mGeoAutoRemovalSearchRequest searchReq;
			SelectionInfoType displayType = SelectionInfoType::GeoEdge;
			if (autoRemoveHole1->isChecked())
			{
				searchReq.searchFaceHoleByRadius = true;
				searchReq.holeMaxRadius = getDefaultValue(Unit::Length, maxHoleRadius->text().toDouble());
			}
			if (autoRemoveHole2->isChecked())
			{
				searchReq.searchFaceHoleByArea = true;
				searchReq.holeMaxArea = maxHoleArea->text().toDouble();
			}
			if (autoRemoveSolidHole->isChecked())
			{
				searchReq.searchSolidHole = true;
				searchReq.solidHoleMaxRadius = getDefaultValue(Unit::Length, maxSolidHoleRadius->text().toDouble());
				displayType = SelectionInfoType::GeoFace;
			}
			if (autoRemoveFillet->isChecked())
			{
				searchReq.searchFillet = true;
				searchReq.filletMaxRadius = getDefaultValue(Unit::Length, maxFilletRadius->text().toDouble());
				displayType = SelectionInfoType::GeoFace;
			}
			if (autoRemoveChamfer->isChecked())
			{
				searchReq.searchChamfer = true;
				searchReq.chamferMaxWidth = getDefaultValue(Unit::Length, maxChamferDis->text().toDouble());
				displayType = SelectionInfoType::GeoFace;
			}
			/*if (autoRemoveThread->isChecked())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("螺纹识别功能目前未开发!"));
				displayType = SelectionInfoType::GeoFace;
			}*/
			if (autoRemoveLogo->isChecked())
			{
				//【集成】dev 侧 logo 识别已实现，识别类型并入面选择（引擎 autoRemovalSearch 承接）
				searchReq.searchLogo = true;
				searchReq.logoMaxHeight = getDefaultValue(Unit::Length, maxLogoHeight->text().toDouble());
				displayType = SelectionInfoType::GeoFace;
			}

			// 调用服务进行识别
			EventDispatcher::mGeoAutoRemovalSearchResult searchRes = getGEOServer.geoEdit().autoRemovalSearchOCC(searchReq);


			std::set<int> geoIds;
			std::set<int> solidHoleFacegeoIds;
			if (displayType == SelectionInfoType::GeoFace)
			{
				// 面类型：实体孔/圆角/斜角/LOGO
				geoIds.insert(searchRes.faceIds.begin(), searchRes.faceIds.end());
				geoIds.insert(searchRes.filletFaceIds.begin(), searchRes.filletFaceIds.end());
				geoIds.insert(searchRes.chamferFaceIds.begin(), searchRes.chamferFaceIds.end());
				geoIds.insert(searchRes.logoFaceIds.begin(), searchRes.logoFaceIds.end());
				solidHoleFacegeoIds = searchRes.allSolidHoleFaceIds;
			}
			else
			{
				// 边类型：面孔
				geoIds.insert(searchRes.edgeIds.begin(), searchRes.edgeIds.end());
			}

			qDebug() << "识别耗时：" << time.elapsed();

			if (geoIds.empty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何!"));
				return false;
			}
			else
			{
				if (SelectionInfoType::GeoFace == displayType)
				{
					MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
				}
				else if(SelectionInfoType::GeoEdge == displayType)
				{
					MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
				}

				std::vector<int> ids;
				ids.insert(ids.end(), geoIds.begin(), geoIds.end());
				geoPickWidget->_featPick = None;
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, displayType, ids);
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();

				geoPickWidget->_featPick = FeaturePick::AutoRemoval;
				autoCollectIds.clear();
				autoCollectIds = geoIds;
				solidHoleFaceIds.clear();
				solidHoleFaceIds = solidHoleFacegeoIds;
				if (autoRemoveSolidHole->isChecked() || autoRemoveFillet->isChecked()||
					autoRemoveChamfer->isChecked() || autoRemoveLogo->isChecked())
				{
					autoRemovalPickFilter = PickFilter::PickGeoFace;
				}
				else if(autoRemoveHole1->isChecked() || autoRemoveHole2->isChecked())
				{
					autoRemovalPickFilter = PickFilter::PickGeoLine;
				}

                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("已高亮符合条件的几何！"));
			}
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("未找到符合条件的几何！"));
			return false;
		}

		return true;
	}
}
