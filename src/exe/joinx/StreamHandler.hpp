#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Note: when path is "-", you will get &cin or &cout
class StreamHandler {
public:
    typedef std::ios_base::openmode openmode;
    StreamHandler();

    // T must be istream, ostream, or iostream
    // we can't just use iostream because that won't work for cin/cout
    template<typename T>
    T* get(const std::string& path);

    // This can be used to wrap a stream in a reader/writer class. The constructor
    // for T is expected to accept T(const string& name, iostream& s)

    template<typename StreamType, typename WrapperType>
    std::shared_ptr<WrapperType> wrap(const std::string& path);

    template<typename StreamType, typename WrapperType>
    std::vector< std::shared_ptr<WrapperType> > wrap(const std::vector<std::string>& paths);

    uint32_t cinReferences() const;
    uint32_t coutReferences() const;

protected:
    struct Stream {
        std::shared_ptr<std::iostream> stream;
        openmode mode;
    };

    std::iostream* getFile(const std::string& path, openmode mode);

protected:
    std::map<std::string, Stream> _streams;
    uint32_t _cinReferences;
    uint32_t _coutReferences;
};

inline uint32_t StreamHandler::cinReferences() const {
    return _cinReferences;
}

inline uint32_t StreamHandler::coutReferences() const {
    return _coutReferences;
}

template<>
inline std::istream* StreamHandler::get<std::istream>(const std::string& path) {
    if (path == "-") {
        ++_cinReferences;
        return &std::cin;
    } else {
        return getFile(path, std::ios::in);
    }
}

template<>
inline std::ostream* StreamHandler::get<std::ostream>(const std::string& path) {
    if (path == "-") {
        ++_coutReferences;
        return &std::cout;
    } else {
        return getFile(path, std::ios::out);
    }
}

template<typename StreamType, typename WrapperType>
inline std::shared_ptr<WrapperType> StreamHandler::wrap(const std::string& path) {
    return std::shared_ptr<WrapperType>(new WrapperType(path, *get<StreamType>(path)));
}

template<typename StreamType, typename WrapperType>
inline std::vector< std::shared_ptr<WrapperType> > StreamHandler::wrap(const std::vector<std::string>& paths) {
    std::vector< std::shared_ptr<WrapperType> > rv;
    for (auto i = paths.begin(); i != paths.end(); ++i)
        rv.push_back(wrap<StreamType, WrapperType>(*i));
    return rv;
}
