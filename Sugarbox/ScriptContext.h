#pragma once

#include <filesystem>

class Emulation;

class ScriptContext
{
public:
   ScriptContext();
   virtual ~ScriptContext();

   void Init(Emulation* emulation);

   void SetVersion(const std::string& version);
   void SetDriveDir(std::filesystem::path path);
   void SetTapeDir(std::filesystem::path path);
   void SetSnapshotDir(std::filesystem::path path);
   void SetSnapshotName(std::string path);
   void SetScreenshotDir(std::filesystem::path path);
   void SetScreenshotName(std::string path);

   void SetKeyDelay(unsigned int delay_press, unsigned int delay, unsigned int delay_cr);

   Emulation* GetEmulation();
   std::filesystem::path GetDriveDir();
   std::filesystem::path GetTapeDir();
   std::filesystem::path GetSnapshotDir();
   std::filesystem::path GetSnapshotName();
   std::filesystem::path GetScreenshotDir();
   std::filesystem::path GetScreenshotName();

   unsigned int GetKeyPressDelay() { return delay_press_; }
   unsigned int GetKeyDelay() { return delay_; }
   unsigned int GetKeyDelayCR() { return delay_cr_; }


   

protected:
   Emulation* emulation_;
   std::string version_;
   std::filesystem::path drive_dir_;
   std::filesystem::path tape_dir_;
   std::filesystem::path snapshot_dir_;
   std::string snapshot_name_;
   std::filesystem::path screenshot_dir_;
   std::string screenshot_name_;

   unsigned int delay_press_;
   unsigned int delay_;
   unsigned int delay_cr_;
};
