#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
namespace store
{
class ImageStore
{
public:
    ImageStore() = default;

    std::string saveImage(std::string image)
    {
        std::string id;
        do
        {
            id = nextId();
        } while(!images_.contains(id) && !id.empty());
        images_[id] = std::move(image);
        return id;
    }

    const std::string& getImageById(const std::string& id)
    {
        return images_[id];
    }

private:
    std::unordered_map<std::string, std::string> images_;

private:

    static std::string nextId()
    {
        static unsigned id = 0;
        return (std::ostringstream() << std::hex << (++id)).str();
    }


};
}