#include "SugarApp.h"
#include "SugarAppFrame.h"



static const int kDefaultWindowWidth = 1280;
static const int kDefaultWindowHeight = 720;

SugarApp::SugarApp() :  wxApp  (), 
   sna_handler_(nullptr),
   display_(nullptr),
   motherboard_(nullptr),
   worker_thread_(nullptr),
   finished_(false)
{

   motherboard_ = new Motherboard (nullptr, &keyboard_handler_);
}

SugarApp::~SugarApp()
{
   delete motherboard_;
}

void EmulationLoop(SugarApp* app)
{
   app->EmulationLoop();
}

void SugarApp::EmulationLoop()
{
   while (true)
   {
      motherboard_->StartOptimizedPlus<true, true, true>(40000);
   }
}

int LoadCprFromBuffer(Motherboard* motherboard, unsigned char* buffer, int size)
{

   // Check RIFF chunk
   int index = 0;
   if (size >= 12
      && (memcmp(&buffer[0], "RIFF", 4) == 0)
      && (memcmp(&buffer[8], "AMS!", 4) == 0)
      )
   {
      // Reinit Cartridge
      motherboard->EjectCartridge();

      // Ok, it's correct.
      index += 4;
      // Check the whole size

      int chunk_size = buffer[index]
         + (buffer[index + 1] << 8)
         + (buffer[index + 2] << 16)
         + (buffer[index + 3] << 24);

      index += 8;

      // 'fmt ' chunk ? skip it
      if (index + 8 < size && (memcmp(&buffer[index], "fmt ", 4) == 0))
      {
         index += 8;
      }

      // Good.
      // Now we are at the first cbxx
      while (index + 8 < size)
      {
         if (buffer[index] == 'c' && buffer[index + 1] == 'b')
         {
            index += 2;
            char buffer_block_number[3] = { 0 };
            memcpy(buffer_block_number, &buffer[index], 2);
            int block_number = (buffer_block_number[0] - '0') * 10 + (buffer_block_number[1] - '0');
            index += 2;

            // Read size
            int block_size = buffer[index]
               + (buffer[index + 1] << 8)
               + (buffer[index + 2] << 16)
               + (buffer[index + 3] << 24);
            index += 4;

            if (block_size <= size && block_number < 256)
            {
               // Copy datas to proper ROM
               unsigned char* rom = motherboard->GetCartridge(block_number);
               memset(rom, 0, 0x1000);
               memcpy(rom, &buffer[index], block_size);
               index += block_size;
            }
            else
            {
               return -1;
            }
         }
         else
         {
            return -1;
         }
      }
   }
   else
   {
      // Incorrect headers
      return -1;
   }

   return 0;
}


bool SugarApp::OnInit()
{
   // Create frame
   auto frame = new SugarAppFrame("Sugarbox", wxPoint(100, 100),
      wxSize(kDefaultWindowWidth, kDefaultWindowHeight));
   frame->Show(true);

   // Create emulation
   motherboard_->SetPlus(true);
   motherboard_->InitMotherbard(nullptr, &sna_handler_, frame->GetDisplay(), nullptr, nullptr, &config_manager_);
   motherboard_->OnOff();
   motherboard_->GetMem()->InitMemory();
   motherboard_->GetMem()->SetRam(1);
   motherboard_->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);
   motherboard_->GetVGA()->SetPAL(true);
   motherboard_->GetPSG()->Reset();
   motherboard_->GetSig()->Reset();
   motherboard_->InitStartOptimizedPlus();
   motherboard_->OnOff();

   // Create dedicated thread for emulation
   EmulationLoop();
   //worker_thread_ = new std::thread( ::EmulationLoop, this);

   return true;
}
