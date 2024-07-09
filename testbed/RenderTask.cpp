#include "RenderTask.h"

#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>

#include <Vulk/internal/debug.h>

#include <filesystem>

namespace {
} // namespace

RenderTask::RenderTask(const Vulk::Context& context) : _context(context) {
}

//
//
//
TextureMappingTask::TextureMappingTask(const Vulk::Context& context) : RenderTask(context) {
}

TextureMappingTask::~TextureMappingTask() {

}

void TextureMappingTask::createFrames() {
}

void TextureMappingTask::prepare() {

}

void TextureMappingTask::render() {

}

//
//
//
PresentTask::PresentTask(const Vulk::Context& context) : RenderTask(context) {

}

PresentTask::~PresentTask() {

}

void PresentTask::prepare() {

}

void PresentTask::render() {

}