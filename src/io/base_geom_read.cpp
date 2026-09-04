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

void Base_Geom_Read::adopt_shape(const TopoDS_Shape &shape,
                                 const QString &description)
{
    clear_loaded_state();
    m_shape = shape;
    m_file_path = description;
    if (!m_shape.IsNull())
    {
        get_bounding_box();
    }
}

void Base_Geom_Read::adopt_loaded_geometry(const Base_Geom_Read &other)
{
    if (this == &other)
    {
        return;
    }

    m_shape = other.m_shape;
    m_hasAssembly = other.m_hasAssembly;
    m_rootShapesCount = other.m_rootShapesCount;
    m_shapeNames = other.m_shapeNames;
    m_last_error_message = other.m_last_error_message;
    m_file_path = other.m_file_path;
    bounding_box = other.bounding_box;
    xyz_min = other.xyz_min;
    xyz_max = other.xyz_max;
    xyz_length = other.xyz_length;
}

bool Base_Geom_Read::report_error(const QString &message)
{
    m_last_error_message = message;
    emit fileReadError(message);
    return false;
}

void Base_Geom_Read::clear_loaded_state()
{
    m_shape.Nullify();
    m_hasAssembly = false;
    m_rootShapesCount = 0;
    m_shapeNames.clear();
    m_last_error_message.clear();
    m_file_path.clear();
    bounding_box.SetVoid();
    xyz_min = QVector3D(0.0f, 0.0f, 0.0f);
    xyz_max = QVector3D(0.0f, 0.0f, 0.0f);
    xyz_length = QVector3D(0.0f, 0.0f, 0.0f);
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
    const QString file_path = QFileDialog::getOpenFileName(
        nullptr,
        "选择文件",
        ".",
        getSupportedFormatsFilter());
    qDebug() << file_path;

    if (file_path.trimmed().isEmpty())
    {
        return false;
    }

    QString mutable_path = file_path;
    const bool ok = readFile(mutable_path);
    if (!ok && !m_last_error_message.trimmed().isEmpty())
    {
        QMessageBox::warning(nullptr, "Geometry Read Error", m_last_error_message);
    }
    return ok;
}

bool Base_Geom_Read::readFile(QString& filePath)
{
    clear_loaded_state();

    if (filePath.trimmed().isEmpty()) {
        return report_error("文件路径为空");
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
    {
        return report_error(QString("文件不存在或不是有效文件: %1").arg(filePath));
    }

    if (fileInfo.size() <= 0)
    {
        return report_error(QString("文件为空，无法读取: %1").arg(filePath));
    }

    QString suffix = fileInfo.suffix().toLower();
    if (suffix.isEmpty())
    {
        return report_error(QString("无法识别文件格式: %1").arg(filePath));
    }

    bool result = false;

    try {
        if (suffix == "step" || suffix == "stp") {
            result = readSTEPFile(filePath);
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
            return report_error("不支持的文件格式: " + suffix);
        }
    }
    catch (const Standard_Failure& e) {
        return report_error("OpenCASCADE异常: " +
                            QString::fromLocal8Bit(e.GetMessageString()));
    }
    catch (...) {
        return report_error("读取几何文件时发生未知异常");
    }

    if (result) {
        if (m_shape.IsNull()) {
            return report_error("成功读取文件但未获取到有效几何数据");
        }
        m_file_path = fileInfo.absoluteFilePath();
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
        return report_error("STEP文件读取失败");
    }

    emit progressUpdate(50);

    // 转换所有根实体
    int nbRoots = reader.NbRootsForTransfer();
    if (nbRoots == 0) {
        return report_error("STEP文件中没有可转换的根实体");
    }

    int nbTransferred = reader.TransferRoots();
    if (nbTransferred == 0) {
        return report_error("STEP文件实体转换失败");
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
        return report_error("IGES文件读取失败");
    }

    emit progressUpdate(50);

    // 转换所有根实体
    reader.TransferRoots();
    m_shape = reader.OneShape();

    emit progressUpdate(100);

    if (m_shape.IsNull()) {
        return report_error("IGES文件转换后未获取到有效形状");
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
        return report_error("STL文件读取失败");
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
        return report_error("BREP文件读取失败");
    }

    m_hasAssembly = false;
    m_rootShapesCount = 1;
    get_bounding_box();
    return true;
}

void Base_Geom_Read::get_bounding_box()
{
    bounding_box.SetVoid();
    BRepBndLib::Add(m_shape,bounding_box);
    if (bounding_box.IsVoid())
    {
        xyz_min={0,0,0};
        xyz_max={0,0,0};
        xyz_length={0,0,0};
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
