#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

#include "ImageEntry.hpp"

namespace store
{
class ImageStore
{
public:
    ImageStore() = default;

    /// @brief Метаданные заполняет вызывающий: он знает источник картинки
    /// и получает формат с размером оттуда, а не угадывает по байтам.
    std::string saveImage(ImageEntry entry)
    {
        std::string id;
        do
        {
            id = nextId();
        } while (images_.contains(id) || id.empty());
        images_[id] = std::move(entry);
        return id;
    }

    const std::string &getImageById(const std::string &id)
    {
        return images_[id].data;
    }

    /// @brief Ищет картинку, не создавая пустую запись. nullptr — id неизвестен.
    const ImageEntry *findImageById(const std::string &id) const
    {
        auto found = images_.find(id);
        return found == images_.end() ? nullptr : &found->second;
    }

    bool contains(const std::string &id) const
    {
        return images_.contains(id);
    }

private:
    std::unordered_map<std::string, ImageEntry> images_;

private:
    static std::string nextId()
    {
        static unsigned id = 0;
        return "img_" + (std::ostringstream() << std::hex << (++id)).str();
    }
};
} // namespace store
