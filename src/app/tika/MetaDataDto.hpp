#pragma once

#include <optional>
#include <string>
#include <vector>

#include <utils/Jsonser.hpp>

namespace tika
{
/// @brief DTO для десериализации JSON-метаданных, генерируемых Apache Tika
struct MetaDataDto
{
    // === Обязательные поля (из запроса) ===

    std::optional<std::string> tkContent        JSONSER_FIELD(tkContent, "tk:content");                            // Текстовый/Markdown контент файла, извлечённый Tika
    std::optional<std::string> contentTypeMagic JSONSER_FIELD(contentTypeMagic, "tk:content-type-magic-detected"); // MIME-тип файла по magic-байтам

    // === Тип файла и формат ===

    std::optional<std::string> contentTypeHeader         JSONSER_FIELD(contentTypeHeader, "Content-Type");                            // MIME-тип из HTTP-заголовка
    std::optional<std::string> dcFormat                  JSONSER_FIELD(dcFormat, "dc:format");                                        // Формат файла из метаданных, например "application/pdf; version=1.7"
    std::optional<std::string> contentTypeParserOverride JSONSER_FIELD(contentTypeParserOverride, "tk:content-type-parser-override"); // Версия формата из переопределения парсера
    std::optional<std::string> contentLength             JSONSER_FIELD(contentLength, "Content-Length");                              // Размер файла в байтах

    // === Временные метки ===

    std::optional<std::string> dateCreated  JSONSER_FIELD(dateCreated, "dcterms:created");   // Дата создания документа
    std::optional<std::string> dateModified JSONSER_FIELD(dateModified, "dcterms:modified"); // Дата последней модификации

    // === Автор и язык ===

    std::optional<std::string> author   JSONSER_FIELD(author, "dc:creator");    // Автор документа
    std::optional<std::string> language JSONSER_FIELD(language, "dc:language"); // Язык документа

    // === Обработка и парсинг ===

    std::optional<std::string> contentHandlerType           JSONSER_FIELD(contentHandlerType, "tk:content-handler-type"); // Тип обработчика контента (MARKDOWN, TEXT и т.д.)
    std::optional<std::string> parseTimeMillis              JSONSER_FIELD(parseTimeMillis, "tk:parse-time-millis");       // Время парсинга в миллисекундах
    std::optional<std::vector<std::string>> parsedBy        JSONSER_FIELD(parsedBy, "tk:parsed-by");                      // Список парсеров Tika, обработавших файл
    std::optional<std::vector<std::string>> parsedByFullSet JSONSER_FIELD(parsedByFullSet, "tk:parsed-by-full-set");      // Полный набор парсеров, включая вложенные
    std::optional<std::string> embeddedDepth                JSONSER_FIELD(embeddedDepth, "tk:embedded-depth");            // Глубина вложенности (0 = корневой файл, 1+ = внутри архива)

    // === Для архивов и вложенных файлов ===

    std::optional<std::string> internalPath JSONSER_FIELD(internalPath, "tk:internal-path"); // Внутренний путь файла внутри архива
    std::optional<std::string> resourceName JSONSER_FIELD(resourceName, "tk:resource-name"); // Имя ресурса
    std::optional<std::string> embeddedId   JSONSER_FIELD(embeddedId, "tk:embedded-id");     // ID встроенного ресурса

    // === Кодировка (для текстовых файлов) ===

    std::optional<std::string> detectedEncoding JSONSER_FIELD(detectedEncoding, "tk:detected-encoding"); // Обнаруженная кодировка файла

    // === Для PDF ===

    std::optional<std::string> pageCount           JSONSER_FIELD(pageCount, "xmpTPg:NPages");    // Количество страниц
    std::optional<std::string> pdfVersion  JSONSER_FIELD(pdfVersion, "pdf:pdf-version"); // Версия PDF
    std::optional<std::string> isEncrypted JSONSER_FIELD(isEncrypted, "pdf:encrypted");  // Зашифрован ли документ

    // === Для изображений ===

    std::optional<std::string> imageWidth  JSONSER_FIELD(imageWidth, "img:Image Width");   // Ширина изображения в пикселях
    std::optional<std::string> imageHeight JSONSER_FIELD(imageHeight, "img:Image Height"); // Высота изображения в пикселях
};
} // namespace tika