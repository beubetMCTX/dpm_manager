#include "base_geom_read.h"

Base_Geom_Read::Base_Geom_Read(QObject *parent)
    : QObject(parent)
    , m_hasAssembly(false)
    , m_rootShapesCount(0)
{


}

Base_Geom_Read::~Base_Geom_Read()
{

}

QString Base_Geom_Read::getSupportedFormatsFilter()
{
    return QStringLiteral(
        "所有支持格式 (*.step *.stp *.iges *.igs *.stl *.brep);;"
        "STEP文件 (*.step *.stp);;"
        "IGES文件 (*.iges *.igs);;"
        "STL文件 (*.stl);;"
        "BREP文件 (*.brep)"
        );
}

bool Base_Geom_Read::Read_Geometry_Dialog()
{
    bool ok=false;
    QString file_path=QFileDialog::getOpenFileName(nullptr, "选择文件", ".","所有文件 (*.*)");
    qDebug()<<file_path;
    ok=readFile(file_path);
    return ok;
}

bool Base_Geom_Read::readFile(QString& filePath)
{
    if (filePath.isEmpty()) {
        emit fileReadError("文件路径为空");
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    bool result = false;

    try {
        if (suffix == "step" || suffix == "stp") {
            // 先尝试用XCAF读取器获取完整结构信息
            // if (readSTEPFileWithStructure(filePath)) {
            //     result = true;
            // } else {
                // 如果XCAF读取失败，回退到基本STEP读取器
                result = readSTEPFile(filePath);
            //}
        }
        else if (suffix == "iges" || suffix == "igs") {
            result = readIGESFile(filePath);
        }
        else if (suffix == "stl") {
            result = readSTLFile(filePath);
        }
        else if (suffix == "brep") {
            result = readBREPFile(filePath);
        }
        else {
            emit fileReadError("不支持的文件格式: " + suffix);
            return false;
        }
    }
    catch (const Standard_Failure& e) {
        QString errorMsg = "OpenCASCADE异常: " +
                           QString::fromLocal8Bit(e.GetMessageString());
        emit fileReadError(errorMsg);
        return false;
    }
    catch (...) {
        emit fileReadError("未知异常发生");
        return false;
    }

    if (result) {
        if (m_shape.IsNull()) {
            emit fileReadError("成功读取文件但未获取到有效几何数据");
            return false;
        }
        emit fileReadSuccess(fileInfo.fileName());
        return true;
    }

    return false;
}

bool Base_Geom_Read::readSTEPFile(const QString& filePath)
{
    STEPControl_Reader reader;

    emit progressUpdate(10);

    IFSelect_ReturnStatus status = reader.ReadFile(filePath.toUtf8().constData());
    if (status != IFSelect_RetDone) {
        emit fileReadError("STEP文件读取失败");
        return false;
    }

    emit progressUpdate(50);

    // 转换所有根实体
    int nbRoots = reader.NbRootsForTransfer();
    if (nbRoots == 0) {
        emit fileReadError("STEP文件中没有可转换的根实体");
        return false;
    }

    int nbTransferred = reader.TransferRoots();
    if (nbTransferred == 0) {
        emit fileReadError("STEP文件实体转换失败");
        return false;
    }

    emit progressUpdate(90);

    m_shape = reader.OneShape();
    m_hasAssembly = (nbRoots > 1);
    m_rootShapesCount = nbRoots;

    emit progressUpdate(100);
    get_bounding_box();
    return true;
}


bool Base_Geom_Read::readIGESFile(const QString& filePath)
{

    IGESControl_Reader reader;

    qDebug()<<"into";

    emit progressUpdate(10);

    IFSelect_ReturnStatus status = reader.ReadFile(filePath.toUtf8().constData());

    if (status != IFSelect_RetDone) {
        emit fileReadError("IGES文件读取失败");
        return false;
    }

    emit progressUpdate(50);

    // 转换所有根实体
    reader.TransferRoots();
    m_shape = reader.OneShape();

    emit progressUpdate(100);

    if (m_shape.IsNull()) {
        emit fileReadError("IGES文件转换后未获取到有效形状");
        return false;
    }

    m_hasAssembly = false;
    m_rootShapesCount = 1;
    get_bounding_box();
    return true;
}

bool Base_Geom_Read::readSTLFile(const QString& filePath)
{
    StlAPI_Reader reader;



    emit progressUpdate(10);

    bool success = reader.Read(m_shape, filePath.toUtf8().constData());

    emit progressUpdate(100);

    if (!success || m_shape.IsNull()) {
        emit fileReadError("STL文件读取失败");
        return false;
    }

    m_hasAssembly = false;
    m_rootShapesCount = 1;
    get_bounding_box();
    return true;
}

bool Base_Geom_Read::readBREPFile(const QString& filePath)
{
    BRep_Builder builder;

    emit progressUpdate(10);

    bool success = BRepTools::Read(m_shape, filePath.toUtf8().constData(), builder);

    emit progressUpdate(100);

    if (!success || m_shape.IsNull()) {
        emit fileReadError("BREP文件读取失败");
        return false;
    }

    m_hasAssembly = false;
    m_rootShapesCount = 1;
    get_bounding_box();
    return true;
}

void Base_Geom_Read::get_bounding_box()
{
    BRepBndLib::Add(m_shape,bounding_box);
    if (bounding_box.IsVoid())
    {
        xyz_min={0,0,0};
        xyz_max={0,0,0};
    }
    else
    {
        double x_min,x_max,y_min,y_max,z_min,z_max;
        bounding_box.Get(x_min,y_min,z_min,x_max,y_max,z_max);

        xyz_min.setX(x_min);
        xyz_min.setY(y_min);
        xyz_min.setZ(z_min);

        xyz_max.setX(x_max);
        xyz_max.setY(y_max);
        xyz_max.setZ(z_max);

        xyz_length.setX(x_max-x_min);
        xyz_length.setY(y_max-y_min);
        xyz_length.setZ(z_max-z_min);

    }

}
