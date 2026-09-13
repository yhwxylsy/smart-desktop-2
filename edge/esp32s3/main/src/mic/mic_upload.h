#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风录音上传（原 main.ino L937-1358 原样搬运）。
//
// 职责：把 WAV 音频以 HTTP 上传到后端 ASR 接口，并解析识别结果。
// 面试可讲：音频较大，采用"multipart 分块上传"——先 POST 建立会话拿到 upload_id，
// 再按块（chunk）循环 POST 音频数据，最后一块带 finalPart 标记，后端拼接后做 ASR。
// 这样内存占用可控，也支持断点/重试语义（每块独立返回 ack）。
bool writeAllToClient(
    WiFiClient &client,
    const uint8_t *data,
    size_t length,
    uint32_t timeoutMs = 15000,
    size_t *writtenOut = nullptr);
String formatWriteFailure(const char *part, size_t written, size_t expected);
int readHttpResponse(WiFiClient &client, String &response);
AsrUploadResult parseAsrUploadResponse(int code, const String &response);
void appendMultipartField(String &body, const String &boundary, const char *name, const String &value);
ChunkUploadResponse parseChunkUploadAck(int code, const String &response, bool finalPart);
ChunkUploadResponse postMicWavChunk(
    const String &uploadId,
    const uint8_t *data,
    size_t chunkSize,
    size_t offset,
    size_t totalAudioSize,
    bool finalPart,
    const String &source,
    bool inject);
AsrUploadResult uploadMicWavChunked(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject);
AsrUploadResult uploadMicWav(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject);
