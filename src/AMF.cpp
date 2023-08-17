//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "AMF.h"
#include <cassert>
#include <netinet/in.h>
#include <stdexcept>

AMFValue::AMFValue(AMFType type) : type_(type) {}

AMFValue::AMFValue(const char *s) {
    type_ = AMF_STRING;
    value_ = new std::string(s);
}

AMFValue::AMFValue(const std::string &s) {
    type_ = AMF_STRING;
    value_ = new std::string(s);
}

AMFValue::AMFValue(double n) {
    type_ = AMF_NUMBER;
    value_ = new double(n);
}

AMFValue::AMFValue(int i) {
    type_ = AMF_NUMBER;
    value_ = new double((double)i);
}

AMFValue::AMFValue(bool b) {
    type_ = AMF_BOOLEAN;
    value_ = new bool(b);
}

AMFValue::AMFValue(const AMFValue &from) : type_(AMF_NULL) { *this = from; }

AMFValue &AMFValue::operator=(const AMFValue &from) {
    if (this == &from) {
        return *this;
    }

    Destroy();
    type_ = from.type_;
    switch (type_) {
        case AMF_STRING:
            value_ = new std::string(*(std::string *)from.value_);
            break;
        case AMF_OBJECT:
        case AMF_ECMA_ARRAY:
            value_ = new ObjectType(*(ObjectType *)from.value_);
            break;
        case AMF_STRICT_ARRAY:
            value_ = new ArrayType(*(ArrayType *)from.value_);
            break;
        case AMF_NUMBER:
            value_ = new double(*(double *)from.value_);
            break;
        case AMF_BOOLEAN:
            value_ = new bool(*(bool *)from.value_);
            break;
        default:
            break;
    }
    return *this;
}

AMFValue::~AMFValue() { Destroy(); }

void AMFValue::Destroy() {
    switch (type_) {
        case AMF_NUMBER:
            delete (double *)value_;
            break;
        case AMF_BOOLEAN:
            delete (bool *)value_;
            break;
        case AMF_STRING:
        case AMF_LONG_STRING:
            delete (std::string *)value_;
            break;
        case AMF_OBJECT:
        case AMF_ECMA_ARRAY:
            delete (ObjectType *)value_;
            break;
        case AMF_STRICT_ARRAY:
            delete (ArrayType *)value_;
        default:
            break;
    }
    value_ = nullptr;
}

AMFType AMFValue::Type() const { return type_; }

void *AMFValue::GetValue() const { return value_; }

void AMFValue::Set(const std::string &s, const AMFValue &val) {
    if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY) {
        printf("AMF not a object");
        return;
    }

    if (!value_) {
        value_ = new ObjectType;
    }
    if (value_) {
        ((ObjectType *)value_)->emplace(s, val);
    }
}

void AMFValue::Add(const AMFValue &val) {
    if (type_ != AMF_STRICT_ARRAY) {
        printf("AMF not a array");
        return;
    }

    if (!value_) {
        value_ = new ArrayType;
    }
    if (value_) {
        ((ArrayType *)value_)->push_back(val);
    }
}
const std::string &AMFValue::AsString() const {
    if (type_ != AMF_STRING || !value_) {
        throw std::runtime_error("AMF not a string");
    }
    return *(std::string *)value_;
}

double AMFValue::AsNumber() const {
    if (type_ != AMF_NUMBER || !value_) {
        throw std::runtime_error("AMF not a number");
    }
    return *(double *)value_;
}

bool AMFValue::AsBoolean() const {
    if (type_ != AMF_BOOLEAN || !value_) {
        throw std::runtime_error("AMF not a boolean");
    }
    return *(bool *)value_;
}

const AMFValue::ObjectType &AMFValue::AsObjectMap() const {
    if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY || !value_) {
        throw std::runtime_error("AMF not a object");
    }

    return *(ObjectType *)value_;
}

AMFValue AMFValue::operator[](const char *key) const {
    if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY) {
        throw std::runtime_error("AMF not a object");
    }

    if (!value_) {
        throw std::runtime_error("AMF data is null");
    }

    auto i = ((ObjectType *)value_)->find(key);
    if (i == ((ObjectType *)value_)->end()) {
        static AMFValue val(AMF_NULL);
        return val;
    }
    return i->second;
}

const AMFValue::ArrayType &AMFValue::AsArray() const {
    if (type_ != AMF_STRICT_ARRAY || !value_) {
        throw std::runtime_error("AMF not a array");
    }

    return *(ArrayType *)value_;
}

/// AMFEncoder
AMFEncoder &AMFEncoder::operator<<(const char *s) {
    if (s) {
        return *this << std::string(s);
    }
    buffer_ += char(AMF_NULL);
    return *this;
}

AMFEncoder &AMFEncoder::operator<<(const std::string &s) {
    if (!s.empty()) {
        buffer_ += char(AMF_STRING);
        assert(s.size() <= 0xffff);
        buffer_ += char((s.size() >> 8) & 0xff);
        buffer_ += char((s.size() & 0xff));
        buffer_ += s;
    } else {
        buffer_ += char(AMF_NULL);
    }
    return *this;
}

AMFEncoder &AMFEncoder::operator<<(std::nullptr_t) {
    buffer_ += char(AMF_NULL);
    return *this;
}

AMFEncoder &AMFEncoder::operator<<(const int n) { return (*this) << (double)n; }

AMFEncoder &AMFEncoder::operator<<(const double n) {
    buffer_ += char(AMF_NUMBER);
    uint8_t value[8];
    for (int i = 0; i < 8; ++i) {
        value[i] = ((uint8_t *)&n)[7 - i];
    }
    buffer_.append((char *)value, 8);
    return *this;
}

AMFEncoder &AMFEncoder::operator<<(const bool b) {
    buffer_ += char(AMF_BOOLEAN);
    buffer_ += char(b);
    return *this;
}

AMFEncoder &AMFEncoder::operator<<(const AMFValue &value) {
    switch (value.Type()) {
        case AMF_STRING:
            *this << *(std::string *)value.GetValue();
            break;
        case AMF_NUMBER:
            *this << *(double *)value.GetValue();
            break;
        case AMF_BOOLEAN:
            *this << *(bool *)value.GetValue();
            break;
        case AMF_NULL:
            *this << nullptr;
            break;
        case AMF_UNDEFINED:
            buffer_ += char(AMF_UNDEFINED);
            break;
        case AMF_OBJECT: {
            buffer_ += char(AMF_OBJECT);
            auto objectMap = *(AMFValue::ObjectType *)value.GetValue();
            for (auto it = objectMap.rbegin(); it != objectMap.rend(); ++it) {
                WriteKay(it->first);
                *this << it->second;
            }

            WriteKay("");
            buffer_ += char(AMF_OBJECT_END);
        } break;
        case AMF_ECMA_ARRAY: {
            buffer_ + char(AMF_ECMA_ARRAY);
            auto objectMap = *(AMFValue::ObjectType *)value.GetValue();
            uint32_t sz = htonl(objectMap.size());
            buffer_.append((char *)&sz, 4);
            for (auto &it : objectMap) {
                WriteKay(it.first);
                *this << it.second;
            }
            WriteKay("");
            buffer_ += char(AMF_OBJECT_END);
        } break;
        case AMF_STRICT_ARRAY: {
            buffer_ += char(AMF_STRICT_ARRAY);
            auto array = *(AMFValue::ArrayType *)value.GetValue();
            uint32_t sz = htonl(array.size());
            buffer_.append((char *)&sz, 4);
            for (auto &val : array) {
                *this << val;
            }
        } break;
        default:
            break;
    }
    return *this;
}

const std::string &AMFEncoder::Data() const { return buffer_; }

void AMFEncoder::Clear() { buffer_.clear(); }

void AMFEncoder::WriteKay(const std::string &key) {
    assert(key.size() <= 0xffff);
    buffer_ += char((key.size() >> 8) & 0xff);
    buffer_ += char((key.size() & 0xff));
    buffer_ += key;
}

/// AMFDecoder
AMFDecoder::AMFDecoder(const uint8_t *buffer, size_t size, int version)
    : buffer_(buffer), pos_(0), size_(size), version_(version) {}

uint8_t AMFDecoder::Front() {
    if (pos_ >= size_) {
        throw std::runtime_error("Not enough data");
    }

    return buffer_[pos_];
}

uint8_t AMFDecoder::PopFront() {
    if (version_ == 0 && Front() == AMF_SWITCH_AMF3) {
        printf("entering AMF3 mode");
        pos_++;
        version_ = 3;
    }

    if (pos_ >= size_) {
        printf("Not enough data\n");
        return 0;
    }

    return buffer_[pos_++];
}

template <>
double AMFDecoder::Load<double>() {
    if (Front() != AMF_NUMBER) {
        throw std::runtime_error("Expected a number");
    }
    pos_++;

    if (pos_ + 8 > size_) {
        throw std::runtime_error("Not enough data");
    }

    uint8_t value[8];
    for (int i = 0; i < 8; ++i) {
        value[7 - i] = buffer_[pos_++];
    }

    return *(double *)value;
}

template <>
bool AMFDecoder::Load<bool>() {
    if (PopFront() != AMF_BOOLEAN) {
        throw std::runtime_error("Expected a boolean");
    }

    return PopFront() != 0;
}

template <>
unsigned int AMFDecoder::Load<unsigned int>() {
    unsigned int value = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t b = PopFront();
        if (i == 3) {
            /* use all bits from 4th byte */
            value = (value << 8) | b;
            break;
        }
        value = (value << 7) | (b & 0x7f);
        if ((b & 0x80) == 0) {
            break;
        }
    }
    return value;
}

template <>
int AMFDecoder::Load<int>() {
    if (version_ == 3) {
        return (int)Load<unsigned int>();
    } else {
        return (int)Load<double>();
    }
}

template <>
std::string AMFDecoder::Load<std::string>() {
    size_t str_len = 0;
    uint8_t type = PopFront();
    if (version_ == 3) {
        str_len = Load<unsigned int>() / 2;
    } else {
        if (type != AMF_STRING) {
            throw std::runtime_error("Expected a string");
        }
        if (pos_ + 2 > size_) {
            throw std::runtime_error("Not enough data");
        }

        str_len = buffer_[pos_] << 8 | buffer_[pos_ + 1];
        pos_ += 2;
    }
    if (pos_ + str_len > size_) {
        throw std::runtime_error("Not enough data");
    }

    std::string s((char *)buffer_ + pos_, str_len);
    pos_ += str_len;
    return s;
}

template <>
AMFValue AMFDecoder::Load<AMFValue>() {
    uint8_t type = Front();
    if (version_ == 3) {
        throw std::runtime_error("Unsupported AMF3 type");
    } else {
        switch (type) {
            case AMF_STRING:
                return AMFValue(Load<std::string>());
            case AMF_NUMBER:
                return AMFValue(Load<double>());
            case AMF_BOOLEAN:
                return AMFValue(Load<bool>());
            case AMF_NULL:
                pos_++;
                return AMFValue(AMF_NULL);
            case AMF_UNDEFINED:
                pos_++;
                return AMFValue(AMF_UNDEFINED);
            case AMF_OBJECT:
                return LoadObject();
            case AMF_ECMA_ARRAY:
                return LoadEcma();
            case AMF_STRICT_ARRAY:
                return LoadArray();
            default:
                throw std::runtime_error("Unsupported AMF type");
        }
    }
}

std::string AMFDecoder::LoadKey() {
    if (pos_ + 2 > size_) {
        throw std::runtime_error("Not enough data");
    }

    size_t str_len = buffer_[pos_] << 8 | buffer_[pos_ + 1];
    pos_ += 2;

    if (pos_ + str_len > size_) {
        throw std::runtime_error("Not enough data");
    }

    std::string s((char *)buffer_ + pos_, str_len);
    pos_ += str_len;
    return s;
}

AMFValue AMFDecoder::LoadObject() {
    AMFValue object(AMF_OBJECT);
    if (Front() != AMF_OBJECT) {
        throw std::runtime_error("Expected an object");
    }
    pos_++;

    while (true) {
        std::string key = LoadKey();
        if (key.empty()) {
            break;
        }
        AMFValue value = Load<AMFValue>();
        object.Set(key, value);
    }
    if (PopFront() != AMF_OBJECT_END) {
        throw std::runtime_error("expected object end");
    }
    return object;
}

AMFValue AMFDecoder::LoadEcma() {
    /* ECMA array is the same as object, with 4 extra zero bytes */
    AMFValue object(AMF_ECMA_ARRAY);
    if (Front() != AMF_ECMA_ARRAY) {
        throw std::runtime_error("Expected an ECMA array");
    }
    pos_++;

    if (pos_ + 4 > size_) {
        throw std::runtime_error("Not enough data");
    }

    pos_ += 4;
    while (true) {
        std::string key = LoadKey();
        if (key.empty()) {
            break;
        }
        AMFValue value = Load<AMFValue>();
        object.Set(key, value);
    }
    if (PopFront() != AMF_OBJECT_END) {
        throw std::runtime_error("expected object end");
    }
    return object;
}

AMFValue AMFDecoder::LoadArray() {
    AMFValue object(AMF_STRICT_ARRAY);
    if (Front() != AMF_STRICT_ARRAY) {
        throw std::runtime_error("Expected an STRICT array");
    }
    pos_++;

    if (pos_ + 4 > size_) {
        throw std::runtime_error("Not enough data");
    }

    int arrSize = buffer_[pos_] << 8 | buffer_[pos_ + 1] | buffer_[pos_ + 2] << 8 | buffer_[pos_ + 3];

    pos_ += 4;
    while (arrSize--) {
        AMFValue value = Load<AMFValue>();
        object.Add(value);
    }

    return object;
}

std::vector<AMFValue> AMFDecoder::GetValues() {
    std::vector<AMFValue> values;
    auto posOld = pos_;
    pos_ = 0;
    while (pos_ < size_) {
        uint8_t type = Front();
        switch (type) {
            case AMF_STRING:
                values.emplace_back(AMFValue(Load<std::string>()));
                continue;
            case AMF_NUMBER:
                values.emplace_back(AMFValue(Load<double>()));
                continue;
            case AMF_BOOLEAN:
                values.emplace_back(AMFValue(Load<bool>()));
                continue;
            case AMF_NULL:
                values.emplace_back(AMFValue(AMF_NULL));
                PopFront();
                continue;
            case AMF_UNDEFINED:
                values.emplace_back(AMFValue(AMF_UNDEFINED));
                PopFront();
                continue;
            case AMF_OBJECT:
                values.emplace_back(LoadObject());
                continue;
            case AMF_ECMA_ARRAY:
                values.emplace_back(LoadEcma());
                continue;
            case AMF_STRICT_ARRAY:
                values.emplace_back(LoadArray());
                continue;
            default:
                throw std::runtime_error("Unsupported AMF type");
        }
    }
    pos_ = posOld; // reset pos
    return values;
}
