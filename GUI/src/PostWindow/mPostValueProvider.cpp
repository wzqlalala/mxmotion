#include "mPostValueProvider.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mPostInfoHub.h"
#include "mPost3DResult.h"

#include "mPostDataBase.h"
#include "mPostResultDataStruct.h"
//包含顺序约束：mIMxdbFile1.h 必须先于 mIONativeFile.h——前者在 PostIOFile 目录内
//同名引索命中本目录 mIOEnum.h（MIOFile 命名空间）钉住 IOFileErrorType；否则
//mIONativeFile.h 的 #include "mIOEnum.h" 在全量构建的 -I 序下会被 src/Common 的
//同名异义文件（MechCommon 命名空间）抢解析，信号声明报 C2039
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"

namespace PostWindow
{
	namespace
	{
		/*
		* 原生格式（OP2/RST·RTH/d3plot/H3D/ODB）适配器：readVarDatas 只有发布式
		* （从常驻 Dataset 按需摊平写库），fetch 按原查询面板口径=发布后回取快照。
		* reader 来自面板注册表（导入期常驻，isModelAlive 由注册表兜底）
		*/
		class NativeValueProvider : public IPostValueProvider
		{
		public:
			explicit NativeValueProvider(MIOFile::mIONativeFile* reader, PostData::mPostDataBase* db)
				: _reader(reader), _db(db) {}

			SourceKind sourceKind() const override { return SourceKind::Native; }

			bool publish(const ValueRequest& req) override
			{
				return _reader != nullptr
					&& _reader->readVarDatas(req.frame, req.var, req.compo, req.complexMode, req.layer);
			}

			bool fetch(const ValueRequest& req, PostData::Compones& out) override
			{
				if (!publish(req))
					return false;
				out = _db->getComponeValues();
				return true;
			}

		private:
			MIOFile::mIONativeFile* _reader;
			PostData::mPostDataBase* _db;
		};

		//VTK 适配器：与 native 同形态（readVarDatas 发布式，fetch 回取快照）
		class VtkValueProvider : public IPostValueProvider
		{
		public:
			explicit VtkValueProvider(MIOFile::mIOVtkFile* reader, PostData::mPostDataBase* db)
				: _reader(reader), _db(db) {}

			SourceKind sourceKind() const override { return SourceKind::Vtk; }

			bool publish(const ValueRequest& req) override
			{
				return _reader != nullptr
					&& _reader->readVarDatas(req.frame, req.var, req.compo, req.complexMode, req.layer);
			}

			bool fetch(const ValueRequest& req, PostData::Compones& out) override
			{
				if (!publish(req))
					return false;
				out = _db->getComponeValues();
				return true;
			}

		private:
			MIOFile::mIOVtkFile* _reader;
			PostData::mPostDataBase* _db;
		};

		/*
		* 老格式 mxdb（.mxdb0+分帧文件）适配器：reader 经面板老格式池取（池键=getFile()
		* 即 .mxdb0，兼作 setInitFileName 入参，池内已配置）；readCloudData 首参用库名。
		* publish 含 ElementNodalForce 4 参特判与磨平应力分支（对齐 slot_showCloud 口径）
		*/
		class MxdbLegacyValueProvider : public IPostValueProvider
		{
		public:
			MxdbLegacyValueProvider(PostData::mPostDataBase* db, mPost3DResultPanel* panel)
				: _db(db), _panel(panel) {}

			SourceKind sourceKind() const override { return SourceKind::MxdbLegacy; }

			bool publish(const ValueRequest& req) override
			{
				MIOFile::mIMxdbFile1* reader = pooledReader();
				if (reader == nullptr || _db == nullptr)
					return false;
				const QString postName = _db->getName();
				if (req.var == QStringLiteral("ElementNodalForce"))
					return reader->readCloudData(postName, req.frame, req.var, req.compo);
				if (req.flattenStress)
					return reader->readFlattenStress(postName, req.frame, req.var, req.compo,
						req.layer, req.complexMode, req.coordinateName, req.userCoord);
				return reader->readCloudData(postName, req.frame, req.var, req.compo,
					req.layer, req.complexMode, req.coordinateName, req.userCoord);
			}

			bool fetch(const ValueRequest& req, PostData::Compones& out) override
			{
				MIOFile::mIMxdbFile1* reader = pooledReader();
				if (reader == nullptr || _db == nullptr)
					return false;
				return reader->readCloudData(_db->getName(), req.frame, req.var, req.compo,
					req.layer, req.complexMode, req.coordinateName, req.userCoord, out);
			}

		private:
			MIOFile::mIMxdbFile1* pooledReader()
			{
				return (_panel != nullptr && _db != nullptr)
					? _panel->mxdbOldReaderFor(_db->getFile())
					: nullptr;
			}

			PostData::mPostDataBase* _db;
			mPost3DResultPanel* _panel;
		};

		/*
		* 新格式 mxdb（601/602 单文件）适配器：reader 经面板新格式池取（首次 open 全文件
		* 扫描建帧索引后复用）；publish 磨平分支无坐标系参数（mMxdbThread 形参面如此）
		*/
		class MxdbNewValueProvider : public IPostValueProvider
		{
		public:
			MxdbNewValueProvider(PostData::mPostDataBase* db, mPost3DResultPanel* panel)
				: _db(db), _panel(panel) {}

			SourceKind sourceKind() const override { return SourceKind::MxdbNew; }

			bool publish(const ValueRequest& req) override
			{
				MIOFile::mMxdbThread* reader = pooledReader();
				if (reader == nullptr || _db == nullptr)
					return false;
				const QString filePath = _db->getFile();
				if (req.flattenStress)
					return reader->readFlattenStress(filePath, req.frame, req.var, req.compo,
						req.layer, req.complexMode, req.userCoord);
				return reader->readVarDatas(filePath, req.frame, req.var, req.compo,
					req.layer, req.complexMode, req.userCoord);
			}

			bool fetch(const ValueRequest& req, PostData::Compones& out) override
			{
				MIOFile::mMxdbThread* reader = pooledReader();
				if (reader == nullptr || _db == nullptr)
					return false;
				return reader->readVarDatas(_db->getFile(), req.frame, req.var, req.compo,
					req.layer, req.complexMode, out, req.userCoord);
			}

		private:
			MIOFile::mMxdbThread* pooledReader()
			{
				return (_panel != nullptr && _db != nullptr)
					? _panel->mxdbNewReaderFor(_db->getFile(), _db)
					: nullptr;
			}

			PostData::mPostDataBase* _db;
			mPost3DResultPanel* _panel;
		};
	}

	IPostValueProvider* valueProviderFor(PostData::mPostDataBase* postDB)
	{
		if (postDB == nullptr)
			return nullptr;
		mPost3DResultPanel* panel = mPostInfoHub::getInstance()->cloudResultPanel().get();
		const QString postFile = postDB->getFile();
		if (panel != nullptr)
		{
			if (MIOFile::mIONativeFile* nativeReader = panel->nativeReaderFor(postFile))
				return new NativeValueProvider(nativeReader, postDB);
			if (MIOFile::mIOVtkFile* vtkReader = panel->vtkReaderFor(postFile))
				return new VtkValueProvider(vtkReader, postDB);
		}
		if (postDB->getDataFlag())
			return new MxdbLegacyValueProvider(postDB, panel);
		return new MxdbNewValueProvider(postDB, panel);
	}
}
