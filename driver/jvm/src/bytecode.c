#include "bytecode.h"

#include "jvm/common.h"
#include "jvm/const.h"
#include "jvm/field.h"
#include "jvm/attrib.h"
#include "jvm/jvm.h"
#include "jvm/method.h"

#include "cthulhu/mediator/driver.h"

#include "base/endian.h"
#include "base/panic.h"
#include "base/memory.h"

#include "scan/scan.h"

#include "report/report.h"

typedef struct jvm_class_t {
    uint16_t minorVersion;
    jvm_version_t majorVersion;
} jvm_class_t;

#define FN_READ(name, type, err) \
    static type name(scan_t *scan) \
    { \
        type value = (err); \
        CTASSERT(scan_read(scan, &value, sizeof(type)) == sizeof(type)); \
        return value; \
    }

FN_READ(read8, uint8_t, UINT8_MAX)
FN_READ(read16, uint16_t, UINT16_MAX)
FN_READ(read32, uint32_t, UINT32_MAX)

static uint8_t kNul = '\0';

static uint16_t read_be16(scan_t *scan)
{
    return native_order16(read16(scan), eEndianBig);
}

static uint32_t read_be32(scan_t *scan)
{
    return native_order32(read32(scan), eEndianBig);
}

static jvm_utf8_info_t utf8_info_read(scan_t *scan)
{
    uint16_t length = read_be16(scan);
    if (length == 0)
    {
        jvm_utf8_info_t value = {
            .length = 0,
            .bytes = &kNul
        };

        return value;
    }

    uint8_t *bytes = ctu_malloc(length + 1);
    CTASSERT(scan_read(scan, bytes, length) == length);
    
    bytes[length] = '\0';

    logverbose("utf8(length=%u, bytes=`%s`)", length, bytes);

    jvm_utf8_info_t value = {
        .length = length,
        .bytes = bytes
    };

    return value;
}

static jvm_class_info_t class_info_read(scan_t *scan)
{
    uint16_t nameIndex = read_be16(scan);

    logverbose("class(nameIndex=%u)", nameIndex);

    jvm_class_info_t value = {
        .nameIndex = nameIndex
    };

    return value;
}

static jvm_field_info_t field_info_read(scan_t *scan)
{
    uint16_t classIndex = read_be16(scan);
    uint16_t nameAndTypeIndex = read_be16(scan);

    logverbose("field(classIndex=%u, nameAndTypeIndex=%u)", classIndex, nameAndTypeIndex);

    jvm_field_info_t value = {
        .classIndex = classIndex,
        .nameAndTypeIndex = nameAndTypeIndex
    };

    return value;
}

static jvm_method_type_info_t method_type_info_read(scan_t *scan)
{
    uint16_t descriptorIndex = read_be16(scan);

    logverbose("methodType(descriptorIndex=%u)", descriptorIndex);

    jvm_method_type_info_t value = {
        .descriptorIndex = descriptorIndex
    };

    return value;
}

static jvm_name_and_type_info_t name_and_type_info_read(scan_t *scan)
{
    uint16_t nameIndex = read_be16(scan);
    uint16_t descriptorIndex = read_be16(scan);

    logverbose("nameAndType(nameIndex=%u, descriptorIndex=%u)", nameIndex, descriptorIndex);

    jvm_name_and_type_info_t value = {
        .nameIndex = nameIndex,
        .descriptorIndex = descriptorIndex
    };

    return value;
}

static jvm_method_handle_info_t method_handle_info_read(scan_t *scan)
{
    uint8_t referenceKind = read8(scan);
    uint16_t referenceIndex = read_be16(scan);

    logverbose("methodHandle(referenceKind=%u, referenceIndex=%u)", referenceKind, referenceIndex);

    jvm_method_handle_info_t value = {
        .referenceKind = referenceKind,
        .referenceIndex = referenceIndex
    };

    return value;
}

static jvm_invoke_dynamic_info_t invoke_dynamic_info_read(scan_t *scan)
{
    uint16_t bootstrapMethodAttrIndex = read_be16(scan);
    uint16_t nameAndTypeIndex = read_be16(scan);

    logverbose("invokeDynamic(bootstrapMethodAttrIndex=%u, nameAndTypeIndex=%u)", bootstrapMethodAttrIndex, nameAndTypeIndex);

    jvm_invoke_dynamic_info_t value = {
        .bootstrapMethodAttrIndex = bootstrapMethodAttrIndex,
        .nameAndTypeIndex = nameAndTypeIndex
    };

    return value;
}

static jvm_float_info_t float_info_read(scan_t *scan)
{
    uint32_t bytes = read_be32(scan);
    float value = *(float *)&bytes;

    // TODO: check this
    logverbose("float(bytes=%u, float=%f)", bytes, value);

    jvm_float_info_t it = {
        .value = bytes
    };

    return it;
}

static jvm_string_info_t string_info_read(scan_t *scan)
{
    uint16_t stringIndex = read_be16(scan);

    logverbose("string(stringIndex=%u)", stringIndex);

    jvm_string_info_t value = {
        .stringIndex = stringIndex
    };

    return value;
}

static jvm_const_t const_read(scan_t *scan)
{
    uint8_t tag = read8(scan);

    jvm_const_t value = {
        .tag = tag
    };

    switch (tag) 
    {
    case eConstUtf8: value.utf8Info = utf8_info_read(scan); break;
    case eConstClass: value.classInfo = class_info_read(scan); break;
    case eConstMethodRef: value.fieldInfo = field_info_read(scan); break;
    case eConstMethodType: value.methodTypeInfo = method_type_info_read(scan); break;
    case eConstNameAndType: value.nameAndTypeInfo = name_and_type_info_read(scan); break;
    case eConstInterfaceMethodRef: value.fieldInfo = field_info_read(scan); break;
    case eConstMethodHandle: value.methodHandleInfo = method_handle_info_read(scan); break;
    case eConstInvokeDynamic: value.invokeDynamicInfo = invoke_dynamic_info_read(scan); break;
    case eConstFloat: value.floatInfo = float_info_read(scan); break;
    case eConstFieldRef: value.fieldInfo = field_info_read(scan); break;
    case eConstString: value.stringInfo = string_info_read(scan); break;

    default: 
        NEVER("unknown tag: %s", jvm_const_tag_string(tag));
    }

    return value;
}

static jvm_attrib_t attrib_read(scan_t *scan)
{
    uint16_t nameIndex = read_be16(scan);
    uint32_t length = read_be32(scan);
    uint8_t *bytes = ctu_malloc(length);
    CTASSERT(scan_read(scan, bytes, length) == length);

    jvm_attrib_t value = {
        .nameIndex = nameIndex,
        .length = length,
        .info = bytes
    };

    return value;
}

static jvm_field_t field_read(scan_t *scan)
{
    jvm_access_t access = read_be16(scan);
    uint16_t nameIndex = read_be16(scan);
    uint16_t descriptorIndex = read_be16(scan);
    uint16_t attributesCount = read_be16(scan);

    jvm_field_t value = {
        .accessFlags = access,
        .nameIndex = nameIndex,
        .descriptorIndex = descriptorIndex,
        .attributesCount = attributesCount,
        .attributes = ctu_malloc(sizeof(jvm_attrib_t) * MAX(attributesCount, 1))
    };

    for (size_t i = 0; i < attributesCount; i++)
    {
        value.attributes[i] = attrib_read(scan);
    }

    return value;
}

static jvm_method_info_t method_read(scan_t *scan)
{
    jvm_access_t access = read_be16(scan);
    uint16_t nameIndex = read_be16(scan);
    uint16_t descriptorIndex = read_be16(scan);
    uint16_t attributesCount = read_be16(scan);

    jvm_method_info_t value = {
        .accessFlags = access,
        .nameIndex = nameIndex,
        .descriptorIndex = descriptorIndex,
        .attributesCount = attributesCount,
        .attributes = ctu_malloc(sizeof(jvm_attrib_t) * MAX(attributesCount, 1))
    };

    for (size_t i = 0; i < attributesCount; i++)
    {
        value.attributes[i] = attrib_read(scan);
    }

    return value;
}

static void class_read(scan_t *scan)
{
    uint16_t minor = read_be16(scan);
    jvm_version_t major = read_be16(scan);

    logverbose("classfile(path=`%s`, major=`%s`, minor=%u)", scan_path(scan), jvm_version_string(major), minor);

    uint16_t constPoolCount = read_be16(scan);
    logverbose("constPool=%u", constPoolCount);

    jvm_const_t *constPool = ctu_malloc(sizeof(jvm_const_t) * constPoolCount);

    for (size_t i = 0; i < constPoolCount - 1; i++)
    {
        constPool[i] = const_read(scan);
    }

    jvm_access_t access = read_be16(scan);
    uint16_t thisClass = read_be16(scan);
    uint16_t superClass = read_be16(scan);

    jvm_const_t thisClassConst = constPool[thisClass - 1];
    jvm_const_t superClassConst = constPool[superClass - 1];

    CTASSERT(thisClassConst.tag == eConstClass);
    CTASSERT(superClassConst.tag == eConstClass);

    jvm_const_t thisClassName = constPool[thisClassConst.classInfo.nameIndex - 1];
    jvm_const_t superClassName = constPool[superClassConst.classInfo.nameIndex - 1];

    CTASSERT(thisClassName.tag == eConstUtf8);
    CTASSERT(superClassName.tag == eConstUtf8);

    logverbose("class(access=%s, thisClass=%s, superClass=%s)", jvm_access_string(access), thisClassName.utf8Info.bytes, superClassName.utf8Info.bytes);

    uint16_t interfaces = read_be16(scan);
    logverbose("interfaces=%u", interfaces);

    for (size_t i = 0; i < interfaces; i++)
    {
        uint16_t idx = read_be16(scan);
        logverbose("interface=%u", idx);
    }

    uint16_t fields = read_be16(scan);
    logverbose("fields=%u", fields);

    for (size_t i = 0; i < fields; i++)
    {
        jvm_field_t field = field_read(scan);
        jvm_const_t name = constPool[field.nameIndex - 1];
        jvm_const_t descriptor = constPool[field.descriptorIndex - 1];
        CTASSERT(name.tag == eConstUtf8);
        CTASSERT(descriptor.tag == eConstUtf8);

        logverbose("field(name=%s, desc=%s)", name.utf8Info.bytes, descriptor.utf8Info.bytes);
        for (size_t i = 0; i < field.attributesCount; i++)
        {
            jvm_attrib_t attrib = field.attributes[i];
            jvm_const_t name = constPool[attrib.nameIndex - 1];
            CTASSERT(name.tag == eConstUtf8);

            logverbose(" - attrib(nameIndex=%u, name=%s, length=%u, bytes=%p)", attrib.nameIndex, name.utf8Info.bytes, attrib.length, attrib.info);
        }
    }

    uint16_t methods = read_be16(scan);
    logverbose("methods=%u", methods);

    for (size_t i = 0; i < methods; i++)
    {
        jvm_method_info_t method = method_read(scan);
        jvm_const_t name = constPool[method.nameIndex - 1];
        jvm_const_t descriptor = constPool[method.descriptorIndex - 1];
        CTASSERT(name.tag == eConstUtf8);
        CTASSERT(descriptor.tag == eConstUtf8);

        logverbose("method(name=%s, desc=%s)", name.utf8Info.bytes, descriptor.utf8Info.bytes);
    
        for (size_t i = 0; i < method.attributesCount; i++)
        {
            jvm_attrib_t attrib = method.attributes[i];
            jvm_const_t name = constPool[attrib.nameIndex - 1];
            CTASSERT(name.tag == eConstUtf8);

            logverbose(" - attrib(name=%s, length=%u, bytes=%p)", name.utf8Info.bytes, attrib.length, attrib.info);
        }
    }

    uint16_t attributes = read_be16(scan);
    logverbose("attributes=%u", attributes);

    for (size_t i = 0; i < attributes; i++)
    {
        jvm_attrib_t attrib = attrib_read(scan);
        jvm_const_t name = constPool[attrib.nameIndex - 1];
        CTASSERT(name.tag == eConstUtf8);

        logverbose("attrib(name=%s, length=%u, bytes=%p)", name.utf8Info.bytes, attrib.length, attrib.info);
    }
}

void jvm_parse(driver_t *driver, scan_t *scan)
{
    lifetime_t *lifetime = handle_get_lifetime(driver);
    reports_t *reports = lifetime_get_reports(lifetime);

    uint32_t magic = read_be32(scan);
    if (magic != 0xCAFEBABE) {
        // TODO: might be a jar file
        report(reports, eWarn, NULL, "`%s` invalid magic number: 0x%x", scan_path(scan), magic);
        return;
    }

    class_read(scan);
}
