#ifndef BASE_GEOM_READ_H
#define BASE_GEOM_READ_H

#include <QObject>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

// OpenCASCADE
#include <TopoDS_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <StlAPI_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <TDocStd_Document.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <qvectornd.h>

#include <BRepBndLib.hxx> // 用于从形状生成边界框
#include <Bnd_Box.hxx>

//using namespace Assimp;

class Base_Geom_Read: public QObject
{
    Q_OBJECT
public:
    explicit Base_Geom_Read(QObject *parent = nullptr);
    ~Base_Geom_Read();

    // 文件读取功能
    bool readFile(QString& filePath);
    TopoDS_Shape getShape() const { return m_shape; }

    // 装配结构相关功能（用于STEP文件）
    bool hasAssemblyStructure() const { return m_hasAssembly; }
    int getNumberOfRootShapes() const { return m_rootShapesCount; }
    QStringList getShapeNames() const { return m_shapeNames; }
    bool Read_Geometry_Dialog();

    // 支持的格式过滤器
    static QString getSupportedFormatsFilter();

    QVector3D xyz_min;
    QVector3D xyz_max;
    QVector3D xyz_length;

signals:
    void fileReadSuccess(const QString& filename);
    void fileReadError(const QString& errorMessage);
    void progressUpdate(int percentage);

private:
    // 具体格式读取实现
    bool readSTEPFile(const QString& filePath);
    bool readIGESFile(const QString& filePath);
    bool readSTLFile(const QString& filePath);
    bool readBREPFile(const QString& filePath);

    void get_bounding_box();


    TopoDS_Shape m_shape;
    bool m_hasAssembly;
    int m_rootShapesCount;
    QStringList m_shapeNames;

    Bnd_Box bounding_box;


};

#endif // BASE_GEOM_READ_H
