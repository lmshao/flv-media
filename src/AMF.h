//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_AMF_H
#define FLV_MEDIA_AMF_H

#include <map>
#include <string>
#include <vector>

enum AMFType : uint8_t {
    AMF_NUMBER = 0,
    AMF_BOOLEAN,
    AMF_STRING,
    AMF_OBJECT,
    AMF_MOVIE_CLIP, // reserved, not supported
    AMF_NULL,
    AMF_UNDEFINED,
    AMF_REFERENCE,
    AMF_ECMA_ARRAY,
    AMF_OBJECT_END,
    AMF_STRICT_ARRAY,
    AMF_DATE,
    AMF_LONG_STRING,
    AMF_UNSUPPORTED,
    AMF_RECORDSET, // reserved, not supported
    AMF_XML_DOCUMENT,
    AMF_TYPED_OBJECT,
    AMF_SWITCH_AMF3
};

template <class T>
struct DisableCompare {
    bool operator()(T lhs, T rhs) const {
        if (lhs == rhs) {
            return false;
        }
        return true;
    }
};

class AMFValue {
public:
    using ObjectType = std::map<std::string, AMFValue, DisableCompare<std::string>>;
    using ArrayType = std::vector<AMFValue>;

    explicit AMFValue(AMFType type = AMF_NULL);
    explicit AMFValue(const char *s);
    explicit AMFValue(const std::string &s);
    explicit AMFValue(double n);
    explicit AMFValue(int i);
    explicit AMFValue(bool b);
    AMFValue(const AMFValue &from);
    AMFValue &operator=(const AMFValue &from);
    ~AMFValue();

    AMFType Type() const;
    void *GetValue() const;
    const std::string &AsString() const;
    double AsNumber() const;
    bool AsBoolean() const;
    const ObjectType &AsObjectMap() const;
    AMFValue operator[](const char *) const;
    const ArrayType &AsArray() const;

    // AMF_OBJECT | AMF_ECMA_ARRAY
    void Set(const std::string &s, const AMFValue &val);
    // AMF_STRICT_ARRAY
    void Add(const AMFValue &val);

    std::string Dump();

private:
    void Destroy();

private:
    AMFType type_;
    void *value_{};
};

class AMFEncoder {
public:
    AMFEncoder &operator<<(const char *s);
    AMFEncoder &operator<<(const std::string &s);
    AMFEncoder &operator<<(std::nullptr_t);
    AMFEncoder &operator<<(int n);
    AMFEncoder &operator<<(double n);
    AMFEncoder &operator<<(bool b);
    AMFEncoder &operator<<(const AMFValue &value);

    const std::string &Data() const;
    void Clear();

private:
    void WriteKay(const std::string &key);

private:
    std::string buffer_;
};

class AMFDecoder {
public:
    AMFDecoder(const uint8_t *buffer, size_t size, int version = 0);

    std::vector<AMFValue> GetValues();

    template <typename T>
    T Load();

private:
    uint8_t Front();
    uint8_t PopFront();
    std::string LoadKey();
    AMFValue LoadObject();
    AMFValue LoadEcma();
    AMFValue LoadArray();

private:
    const uint8_t *buffer_;
    size_t pos_;
    size_t size_;
    int version_;
};

#endif // FLV_MEDIA_AMF_H
