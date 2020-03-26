#include "SoundControl.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

SoundControl::SoundControl(ISoundNotification* sound_notification, MultiLanguage* language) :sound_notification_(sound_notification), language_(language)
{
}

SoundControl::~SoundControl()
{

}

void SoundControl::DrawSoundVolume()
{
   float v = sound_notification_->GetVolume();

   // 200 pixel wide
   ImGui::SetNextItemWidth(200.0f);
   if (ImGui::SliderFloat(language_->GetString("L_Volume"), &v, 0.0f, 1.0f))
   {
      // Update soud volume
      sound_notification_->SetVolume(v);
   }
};