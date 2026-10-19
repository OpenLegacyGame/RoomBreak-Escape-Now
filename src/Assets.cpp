#include "Assets.h"
#include <stdexcept>
namespace rb
{
std::optional<std::string> findOtherText(int language, std::string_view key)
{
    if (language < 0 || language > 3)
        throw std::runtime_error("Invalid text language");
    const char* languages[] = {"chi", "kor", "eng", "jap"};
    auto file               = ax::FileUtils::getInstance()->getStringFromFile(
        (language == 1 ? std::string("other_texts.txt") : std::string("other_texts_") + languages[language] + ".txt"));
    file.erase(std::remove(file.begin(), file.end(), '\r'), file.end());
    auto start = file.find("/" + std::string(key) + "\n");
    if (start == std::string::npos)
        return std::nullopt;
    start          = file.find('\n', start) + 1;
    const auto end = file.find("\n/", start);
    auto text      = file.substr(start, end == std::string::npos ? end : end - start);
    while (!text.empty() && text.back() == '\n')
        text.pop_back();
    return text;
}
std::string otherText(int language, std::string_view key)
{
    auto text = findOtherText(language, key);
    if (!text)
        throw std::runtime_error("Missing other text: " + std::string(key));
    return std::move(*text);
}
ax::Vec2 topLeft(ax::Node* parent, ax::Node* child, float x, float y)
{
    const auto size = child->getContentSize();
    return {x + size.width / 2, parent->getContentSize().height - y - size.height / 2};
}
bool hitTest(ax::Node* node, ax::Vec2 world)
{
    if (!ax::Rect(ax::Vec2::ZERO, node->getContentSize()).containsPoint(node->convertToNodeSpace(world)))
        return false;
    for (auto* parent = node->getParent(); parent; parent = parent->getParent())
        if (auto* clip = dynamic_cast<ax::ClippingRectangleNode*>(parent); clip && clip->isClippingEnabled())
            if (!clip->getClippingRegion().containsPoint(clip->convertToNodeSpace(world)))
                return false;
    return true;
}
const char* font(int language)
{
    const char* fonts[] = {"fonts/NotoSansCJKsc-Regular.ttf", "fonts/NotoSansKR-Regular.ttf", "fonts/arial.ttf",
                           "fonts/NotoSansJP-Regular.otf"};
    if (language < 0 || language > 3)
        throw std::runtime_error("Invalid font language");
    return fonts[language];
}
ax::Label* label(std::string_view text, int language, float size)
{
    ax::TTFConfig config(font(language), size);
    // Keep logical text dimensions while sampling a detailed outline. SDF
    // reconstructs the edge at framebuffer resolution, including typewriter zoom.
    config.distanceFieldEnabled = true;
    config.faceSize             = 64;
    auto* value                 = ax::Label::createWithTTF(config, text);
    if (!value)
        throw std::runtime_error("Cannot create dialogue font");
    return value;
}
ax::Sprite* sprite(std::string_view filename)
{
    std::string path(filename);
    bool hd     = false;
    auto exists = [](const std::string& name) {
        auto* f  = ax::FileUtils::getInstance();
        auto dot = name.rfind('.');
        return f->isFileExist(name) ||
               (dot != std::string::npos && f->isFileExist(name.substr(0, dot) + "-hd" + name.substr(dot)));
    };
    if (auto chi = path.find("_chi"); chi != std::string::npos && !exists(path))
    {
        auto english = path;
        english.replace(chi, 4, "_eng");
        if (exists(english))
            path = english;
    }
    if (auto dot = path.rfind('.'); dot != std::string::npos)
    {
        auto candidate = path.substr(0, dot) + "-hd" + path.substr(dot);
        if (ax::FileUtils::getInstance()->isFileExist(candidate))
        {
            path = candidate;
            hd   = true;
        }
    }
    auto* value = ax::Sprite::create(path);
    if (!value)
        throw std::runtime_error("Missing original asset: " + path);
    if (hd)
    {
        value->setStretchEnabled(true);
        value->setContentSize(value->getContentSize() / 2);
    }
    return value;
}
void attach(ax::Node* parent, ax::Node* node, float x, float y, int z)
{
    parent->addChild(node, z);
    node->setPosition(topLeft(parent, node, x, y));
}
ax::Sprite* place(ax::Node* parent, std::string_view file, float x, float y, int z)
{
    auto* value = sprite(file);
    attach(parent, value, x, y, z);
    return value;
}
void rotateGear(ax::Node* node, float seconds, bool clockwise)
{
    using namespace ax;
    node->runAction(RepeatForever::create(Sequence::create(RotateTo::create(seconds, clockwise ? 120 : 240),
                                                           RotateTo::create(seconds, clockwise ? 240 : 120),
                                                           RotateTo::create(seconds, clockwise ? 0 : 360), nullptr)));
}
}  // namespace rb
