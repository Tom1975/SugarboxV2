
#include "ScriptContext.h"
#include "Emulation.h"

ScriptContext::ScriptContext() : emulation_(nullptr), delay_(0), delay_cr_(0)
{
   
}

ScriptContext::~ScriptContext()
{
   
}

void ScriptContext::Init(Emulation* emulation)
{
   emulation_ = emulation;
}

void ScriptContext::SetVersion(const std::string& version)
{
   version_ = version;
}

Emulation* ScriptContext::GetEmulation()
{
   return emulation_;
}

void ScriptContext::SetDriveDir(std::filesystem::path path)
{
   drive_dir_ = path;
}

std::filesystem::path ScriptContext::GetDriveDir()
{
   return drive_dir_;
}

void ScriptContext::SetTapeDir(std::filesystem::path path)
{
   tape_dir_ = path;
}

std::filesystem::path ScriptContext::GetTapeDir()
{
   return tape_dir_;
}

void ScriptContext::SetSnapshotDir(std::filesystem::path path)
{
   snapshot_dir_= path;
}

std::filesystem::path ScriptContext::GetSnapshotDir()
{
   return snapshot_dir_;
}

void ScriptContext::SetSnapshotName(std::string path)
{
   snapshot_name_ = path;
}

std::filesystem::path ScriptContext::GetSnapshotName()
{
   return snapshot_dir_;
}

void ScriptContext::SetScreenshotDir(std::filesystem::path path)
{
   screenshot_dir_ = path;
}

std::filesystem::path ScriptContext::GetScreenshotDir()
{
   return screenshot_dir_;
}

void ScriptContext::SetScreenshotName(std::string path)
{
   screenshot_name_ = path;
}

std::filesystem::path ScriptContext::GetScreenshotName()
{
   return screenshot_name_;
}

void ScriptContext::SetKeyDelay(unsigned int delay_press, unsigned int delay, unsigned int delay_cr)
{
   delay_ = delay;
   delay_press_ = delay_press;
   delay_cr_ = delay_cr_;
}
