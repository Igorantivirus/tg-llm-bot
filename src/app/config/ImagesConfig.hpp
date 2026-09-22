#pragma once

#include <string>

namespace config
{

/// @brief Размеры генерируемых картинок под каждое соотношение сторон.
///
/// Значения зависят от модели: диффузионные модели обучены на конкретных
/// разрешениях и вне их дают артефакты, поэтому набор задаётся в конфиге,
/// а не зашит в инструмент. Формат строки — "ШИРИНАxВЫСОТА".
struct AspectRatios
{
    std::string square = "1024x1024";
    std::string portrait = "832x1216";
    std::string landscape = "1216x832";
};

struct ImagesConfig
{
    /// Модель для картинок по умолчанию; в каждом чате её можно сменить командой.
    std::string  defaultModel = "qwen-edit-nsfw";
    AspectRatios aspectRatios;
};

} // namespace config
