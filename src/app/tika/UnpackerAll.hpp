#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <charconv>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

#include <miniz/miniz.h>
#include <unordered_map>

#include "AllUnpackedData.hpp"
#include "Error.hpp"
#include "MetaDataDto.hpp"
#include "net/HttpClient.hpp"
#include "net/HttpResponse.hpp"
#include "net/Types.hpp"
#include "utils/Parser.hpp"
#include "utils/Types.hpp"

namespace tika
{
class UnpackerAll
{
private:
    struct FilePairInfo
    {
        static constexpr mz_uint invalidMzUint = std::numeric_limits<mz_uint>::max();
        mz_uint                  metaId = invalidMzUint;
        mz_uint                  fileId = invalidMzUint;
        std::string              fileName;
        std::size_t              metaSize;
        std::size_t              fileSize;
        bool                     isFilled() const
        {
            return metaId != invalidMzUint && fileId != invalidMzUint && !fileName.empty();
        }
    };

public:
    UnpackerAll(asio::any_io_executor ex, const std::size_t count)
        : http_(std::move(ex), count)
    {
    }

    utils::AsyncResult<AllUnpackedData> unpackAll(std::string binaryFile, std::string fileName)
    {
        auto binEx = co_await getBinaryZip(std::move(binaryFile), std::move(fileName));
        if (!binEx)
            co_return std::unexpected(binEx.error());

        AllUnpackedData res;
        auto            packRes = initUnpackedData(binEx.value(), res);
        if (!packRes)
            co_return std::unexpected(packRes.error());
        co_return res;
    }

private:
    net::HttpClient http_;

private:
    // "%d.*.metadata.json"
    static std::optional<int> isMetaDataFileName(const std::string_view s)
    {
        int value;
        auto [end, err] = std::from_chars(s.data(), s.data() + s.size(), value);
        if (end >= s.data() + s.size() || *end != '.')
            return std::nullopt;
        if (s.ends_with(".metadata.json"))
            return value;
        return std::nullopt;
    }
    // "%d.*"
    static std::optional<int> getFileIndex(const std::string_view s)
    {
        int value;
        auto [end, err] = std::from_chars(s.data(), s.data() + s.size(), value);
        if (end < s.data() + s.size() && *end == '.')
            return value;
        return std::nullopt;
    }

private:
    void fillFilesInfo(std::unordered_map<int, FilePairInfo> &files, mz_zip_archive &zipArchive)
    {
        mz_uint filesCount = mz_zip_reader_get_num_files(&zipArchive);
        mz_uint capacity = filesCount / 2 + filesCount % 2;
        files.reserve(capacity);

        for (mz_uint i = 0; i < filesCount; ++i)
        {
            mz_zip_archive_file_stat fileStat;
            if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat))
            {
                std::cerr << "Не удалось прочитать информацию о файле " << i << '\n';
                continue;
            }
            if (auto index = isMetaDataFileName(fileStat.m_filename); index)
            {
                FilePairInfo &info = files[index.value()];
                info.metaId = i;
                info.metaSize = fileStat.m_uncomp_size;
            }
            else if (auto index = getFileIndex(fileStat.m_filename); index)
            {
                FilePairInfo &info = files[index.value()];
                info.fileId = i;
                info.fileSize = fileStat.m_uncomp_size;
                info.fileName = fileStat.m_filename;
            }
            else
                std::cerr << "Неподдерживаемый файл: " << fileStat.m_filename << '\n';
        }
    }

    std::optional<std::string> readFile(mz_zip_archive &zipArchive, mz_uint index, std::size_t size)
    {
        std::string fileData(size, '\0');
        if (mz_zip_reader_extract_to_mem(&zipArchive, index, reinterpret_cast<char *>(fileData.data()), size, 0))
            return fileData;
        return std::nullopt;
    }

    utils::SyncResult<void> initUnpackedData(std::string &binFile, AllUnpackedData &data)
    {
        mz_zip_archive zipArchive;
        std::memset(&zipArchive, 0, sizeof(zipArchive));
        if (!mz_zip_reader_init_mem(&zipArchive, binFile.data(), binFile.size(), 0))
        {
            std::cerr << "Ошибка открытия ZIP из памяти!" << '\n';
            return std::unexpected(Error::ZipOpen);
        }
        std::unordered_map<int, FilePairInfo> files; // {id_in_zip: {}}
        fillFilesInfo(files, zipArchive);

        for (auto &&[id, pair] : files)
        {
            if (!pair.isFilled())
                continue;

            auto metaFile = readFile(zipArchive, pair.metaId, pair.metaSize);
            if (!metaFile)
                continue;
            auto dtoEx = utils::deserialize<MetaDataDto>(metaFile.value());
            if (!dtoEx)
                continue;
            MetaDataDto &dto = dtoEx.value();
            if (dto.contentTypeMagic && dto.contentTypeMagic->starts_with("image/") && dto.imageHeight && dto.imageWidth) // Имеем дело с кратинкой
            {
                auto imageBin = readFile(zipArchive, pair.fileId, pair.fileSize);
                if (!imageBin)
                    continue;
                ImageData image;
                try
                {
                    image.width = std::stoi(dto.imageWidth.value());
                    image.height = std::stoi(dto.imageHeight.value());
                }
                catch(...)
                {
                    std::cerr << "stoi error\n";
                    continue;
                }
                image.type = dto.contentTypeMagic.value();
                image.fileName = dto.resourceName.value_or("unknown_image");
                image.data = std::move(imageBin.value());
                data.imageFiles.push_back(std::move(image));
            }
            else if (dto.tkContent) // просто какой-то файл
            {
                FileData file;
                file.type = dto.contentTypeMagic.value_or("unknown/type");
                file.fileName = dto.resourceName.value_or("unknown_file");
                file.data = dto.tkContent.value();
                data.textParsedFiles.push_back(std::move(file));
            }
        }

        mz_zip_reader_end(&zipArchive);
        return utils::empty;
    }

    utils::AsyncResult<std::string> getBinaryZip(std::string binaryFile, std::string fileName)
    {
        net::BeastRequest req(http::verb::put, "/unpack/all", 11);
        req.body() = std::move(binaryFile);
        initRequestFields(req, std::move(fileName));

        auto resp = co_await http_.request("localhost", "9998", std::move(req));
        if (!resp)
            co_return std::unexpected(resp.error());
        net::HttpResponse response = std::move(resp.value());
        if (response.header.result_int() / 100 != 2)
        {
            std::cout << "Tika Error with code " << response.header.result_int() << '\n';
            co_return std::unexpected(Error::TikaError);
        }

        co_return response.isStreaming() ? co_await response.streamBody().readAll() : std::move(response.stringBody());
    }

    void initRequestFields(net::BeastRequest &req, std::string fileName) const
    {
        req.set(http::field::host, "localhost:9998");
        fileName.insert(0, "attachment; filename=");
        req.set(http::field::content_disposition, std::move(fileName));
        req.prepare_payload();
    }
};
} // namespace tika