#include "Germanium_Detector_System.h"

#include <cstdlib>
#include <map>

#include "FEBEX.h"
#include "Configuration_Files/DESPEC_General_Setup/DESPEC_Setup_File.h"



using namespace std;

//---------------------------------------------------------------

Germanium_Detector_System::Germanium_Detector_System(){

    //set amount of Germanium Detectors

  max_am_dets = Germanium_MAX_HITS;

    fired_FEBEX_amount = 0;
   
    Sum_Time = new ULong64_t[max_am_dets];
    Hit_Pattern = new int[max_am_dets];

    det_ids = new int[max_am_dets];
    crystal_ids = new int[max_am_dets];
    Chan_Time = new ULong64_t[max_am_dets];
    Chan_Energy = new double[max_am_dets];
    Chan_CF = new ULong64_t[max_am_dets];
    Pileup = new bool[max_am_dets];
    Overflow = new bool[max_am_dets];

    Germanium_T_CALIB = new Germanium_Time_Calibration();
    Germanium_E_CALIB = new Germanium_Energy_Calibration();
       if(Germanium_TRACES_ACTIVE){
//     trace_channel_id= new int[max_am_dets];
//     trace_board_id= new int[max_am_dets];
    l_dat_fir= new int*[max_am_dets];
    l_dat_sec= new int*[max_am_dets];
    for(int i=0; i<max_am_dets; i++){
        l_dat_fir[i]= new int[Germanium_TRACE_LENGTH/2];
        l_dat_sec[i]= new int[Germanium_TRACE_LENGTH/2];
            }
      }

    load_board_channel_file();

  
}

//---------------------------------------------------------------

Germanium_Detector_System::~Germanium_Detector_System(){

  //Germanium_map.clear(); - not necessary
    
    delete[] det_ids;
    delete[] crystal_ids;
    delete[] Sum_Time;
    delete[] Pileup;
    delete[] Hit_Pattern;
    delete[] Chan_Time;
    delete[] Chan_Energy;
    delete[] Chan_CF;
    delete[] Pileup;
    delete[] Overflow;
    
      if(Germanium_TRACES_ACTIVE){
//     delete[] trace_channel_id;
//     delete[] trace_board_id;
    
    for(int i=0; i < Germanium_TRACE_LENGTH/2; ++i){
    delete[] l_dat_fir[i];
    delete[] l_dat_sec[i];
    
    }
    delete[] l_dat_fir; 
    delete[] l_dat_sec; 
      }
    delete Germanium_T_CALIB;
    delete Germanium_E_CALIB;
}

//---------------------------------------------------------------

void Germanium_Detector_System::load_board_channel_file()
{
  ifstream file("Configuration_Files/Germanium/Germanium_Detector_Map.txt");
    cout<<"Loading Germanium detector map" << endl;
  if(file.fail())
  {
    cerr << "Could not find FEBEX Germanium Mapping File!" << endl;
        exit(0);
    }

  constexpr auto ignore = std::numeric_limits<std::streamsize>::max();

  while(file.good())
  {
    if (file.peek() == '#')
    {
      file.ignore(ignore, '\n');
      
      continue;
    }
    //#Board ID   Ch ID    Det Num    Crysal Num
    int board_id, channel_num, det_num, crystal_num;
    // file.ignore(ignore, '\t');
    file >> board_id >> channel_num >> det_num >> crystal_num;
    
   
    file.ignore(ignore, '\n');

    // Map board,channel to detector,crystal
    Germanium_map[std::make_pair(board_id,channel_num)] = std::make_pair(det_num, crystal_num);
   // cout<<"board_id " << board_id << " channel_num " << channel_num <<" det_num " <<det_num << "  crystal_num  "<<crystal_num <<" Germanium_map.size() "<<Germanium_map.size() <<endl;
    }
}

//---------------------------------------------------------------

void Germanium_Detector_System::get_Event_data(Raw_Event* RAW){
    
  RAW->set_DATA_Germanium(fired_FEBEX_amount,Sum_Time,Ge_channels,Chan_Time,Chan_Energy,det_ids,crystal_ids,Pileup,Overflow, Chan_CF);
    if(Germanium_TRACES_ACTIVE){
 // RAW->set_DATA_Germanium_Traces(fired_FEBEX_amount, l_trace_size,trace_board_id, trace_channel_id, l_dat_fir,l_dat_sec);
    RAW->set_DATA_Germanium_Traces(fired_FEBEX_amount, l_trace_size, l_dat_fir,l_dat_sec);
    }
}

//---------------------------------------------------------------

void Germanium_Detector_System::Process_MBS(int* pdata){

    reset_fired_channels();
    
 // std::pair<int,int> current_det;
    
    this->pdata = pdata;
    
   
    
    bool FEBEX_data_loop = true;
    
  int num_modules = Germanium_FEBEX_MODULES; // Number of modules in the data //
    
    fired_FEBEX_amount = 0;
    
    FEBEX_Add* FEBEX_add  = (FEBEX_Add*) this->pdata;

  while (FEBEX_add->add == 0xadd){
        this->pdata++;
        FEBEX_add = (FEBEX_Add*) this->pdata;
   
    }

    FEBEX_Header* FEBEXhead  = (FEBEX_Header*) this->pdata;

    while(FEBEX_data_loop){
//  printf("ALLpdata=0x%08x\n",*this->pdata++);
    if (FEBEXhead->ff == 0xFF){ // FEBEX module idicator //

            // FEBEXhead->ff;
            // FEBEXhead->chan_head;
            // FEBEXhead->three_four;
    
            board_id = FEBEXhead->chan_head;
           // cout<<"board_id " <<board_id << endl;
            
            this->pdata++; // Moves to Channel Size //
            
            FEBEX_Chan_Size *fbx_size=(FEBEX_Chan_Size*) this->pdata;    
            
            num_channels = ((fbx_size->chan_size)/4) - 1;
    
            if (num_channels == 0) num_modules--;
        
            this->pdata++; // Moves to Event Timestamp //

            FEBEX_Half_Time *fbx_hT=(FEBEX_Half_Time*) this->pdata;
        
            this->pdata++; // Moves to rest of Event Timestamp //
            
            FEBEX_Evt_Time *fbx_time=(FEBEX_Evt_Time*) this->pdata;

            ULong64_t tmp_ext_time = ((fbx_hT->ext_time));
            
            tmp_Sum_Time = (fbx_time->evt_time)+ (tmp_ext_time<<32);//((fbx_hT->ext_time)<<32);
            //printf("SumTime_pdata=0x%08x\n",*this->pdata);
            //cout<<"1 tmp_Sum_Time " <<tmp_Sum_Time << " fbx_time->evt_time " <<fbx_time->evt_time <<endl;
            this->pdata++; // Moves to Pileup & Hit Pattern //
                        
            FEBEX_Flag_Hits *fbx_flag=(FEBEX_Flag_Hits*) this->pdata;
        
            tmp_Pileup = fbx_flag->pile_flags;
        
            tmp_Hit_Pattern = fbx_flag->hit_pattern;
        
      for(int j = 15; j >= 0; j--)
      {
        if(tmp_Pileup & (1 << j)) {
                    pileup_flags[j] = 1;
                }
        if(tmp_Hit_Pattern & (1 << j)) {
                    Ge_channels[j] = j;
                    num_channels_fired++;
                }
            }
            this->pdata++; // Moves to DEADBEEF //
             //printf("Here is deadbeef=0x%08x\n",*this->pdata);
        }
    else if (FEBEXhead->ff == 0xF0){ // FEBEX channel indicator //( (l_dat & 0xff) == 0x34)  // channel header
            this->pdata--; // Moves back to DEADBEEF so channel loop functions properly //
        
         //  cout<<"num_channels " << num_channels << endl;
            for(int i=0; i<num_channels; ++i){
                this->pdata++; // Moves to channel header //
                //printf("Here at Chan=0x%08x\n",*this->pdata);
          
                FEBEX_Chan_Header *fbx_Ch=(FEBEX_Chan_Header*) this->pdata;
        
                int tmp_Ch_ID = fbx_Ch->Ch_ID;
     
        //if(false && pileup_flags[tmp_Ch_ID] == 1) this->pdata += 3;
            
        //else
        {
    
          auto idx = std::make_pair(board_id, tmp_Ch_ID);
           
         // cout<<"board_id " <<board_id << " tmp_Ch_ID " <<tmp_Ch_ID <<endl;
          if(Germanium_map.find(idx) != Germanium_map.end())
          {
            
            //det_ids[i] = current_det;
            Sum_Time[fired_FEBEX_amount] = tmp_Sum_Time;
	   // printf("Upper pdata=0x%08x\n",*this->pdata);
            this->pdata++; // Moves to rest of channel timestamp //
           
            
	  // printf("Lower pdata=0x%08x\n",*this->pdata);
	   
            FEBEX_TS *fbx_Ch_TS=(FEBEX_TS*) this->pdata; 
            ULong64_t tmp_ext_chan_ts = (fbx_Ch->ext_chan_ts);
	   
            Chan_Time[fired_FEBEX_amount] = ((fbx_Ch_TS->chan_ts)+(tmp_ext_chan_ts<<32))*10; // in nanoseconds
            
        
	   
	   this->pdata++; // Moves to Channel Energy //
  
            FEBEX_En *fbx_Ch_En=(FEBEX_En*) this->pdata; 
            
            Chan_Energy[fired_FEBEX_amount] = fbx_Ch_En->chan_en;
            Chan_CF[fired_FEBEX_amount] = 10.0*((fbx_Ch_TS->chan_ts)+(fbx_Ch_En->cf)/64.0);
            
            //if(Chan_CF[fired_FEBEX_amount]!=0)cout<<"Chan_CF[fired_FEBEX_amount] " <<Chan_CF[fired_FEBEX_amount]<< endl;
                       // Sign extend the 24-bit energy so negative energies work properly
            if (fbx_Ch_En->chan_en & 0x00800000)
            {
              int energy = 0xFF000000 | fbx_Ch_En->chan_en;
              Chan_Energy[fired_FEBEX_amount] = energy;
            }
            Pileup[fired_FEBEX_amount] = fbx_Ch_En->pileup != 0;
            Overflow[fired_FEBEX_amount] = fbx_Ch_En->overflow != 0;
            det_ids[fired_FEBEX_amount] = Germanium_map[idx].first;
            crystal_ids[fired_FEBEX_amount] = Germanium_map[idx].second;
           
            //cout<<"det_ids[fired_FEBEX_amount] " <<det_ids[fired_FEBEX_amount] << " crystal_ids[fired_FEBEX_amount] "<< crystal_ids[fired_FEBEX_amount] << " tmp_Ch_ID " <<tmp_Ch_ID <<" board_id "<<board_id<<endl;
            //cout<<" det_ids[fired_FEBEX_amount] " <<  det_ids[fired_FEBEX_amount] <<" crystal_ids[fired_FEBEX_amount] "<< crystal_ids[fired_FEBEX_amount]<< " fired_FEBEX_amount " << fired_FEBEX_amount <<endl;
            //cout <<  "1) Chan_Energy[current_det] "<<  Chan_Energy[fired_FEBEX_amount] << " current_det " <<  det_ids[fired_FEBEX_amount] <<endl;
            //Calibrate_FEBEX(current_det);
            //
            //std::cout << "pdata = " << std::hex << *this->pdata << std::dec << std::endl;
            //if ((*this->pdata & 0x80000000) != 0) cout<<" Bit 31 set "<<endl;
            //if ((*this->pdata & 0x40000000) != 0) cout<<" Bit 30 set "<<endl;
           // std::cout << "Pileup = " << fbx_Ch_En->pileup << ", OF = " << fbx_Ch_En->overflow << std::endl;
    
            //this->pdata++; // Moves to Traces Use //
        //    printf(" I am before future skip to traces_pdata=0x%08x\n",*this->pdata);
            this->pdata++; // Moves to Future Use //
            //printf("Before I go to traces_pdata=0x%08x\n",*this->pdata);
            fired_FEBEX_amount++;
        
                }///This is the mapping loop
                 
                 /// If mapping doesnt fit 
                 else 
          {
           
           //std::cout << "Unknown detector with channel: " << board_id << ", " << tmp_Ch_ID << std::endl;
            this->pdata += 3; /// Moves to Future Use 
  
                        }
                    } ///this is the else condition 
            } ///This is the channel loop 
             
       num_modules--;
     
            /// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ //
            /// @@@@ Traces Go Here @@@@@ //
            /// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ //
    if(Germanium_TRACES_ACTIVE){
 
            for(int i=0; i<num_channels; i++){

           this->pdata++;
          
           if((*this->pdata & 0xFF) == 0x00000034){

                // trace_board_id[fired_FEBEX_amount]    = (*this->pdata & 0xff0000)   >> 16;
               //  trace_channel_id[fired_FEBEX_amount] = (*this->pdata & 0xff000000) >> 24;  
                
          ///Go to trace data structure         
           this->pdata++;
             l_cha_size = *this->pdata++; 
             l_trace_head = *this->pdata++;
       
            
            if ( ((l_trace_head & 0xf0000000) >> 24) != 0xa0)
        {
        //  printf ("ERROR>> trace header id is not 0xaa (0x%08x)\n",l_trace_head );
          continue;  
          
        }
            
            l_trace_size = (l_cha_size/4) - 2;     /// in longs/32bit
          
         
            for (int l_l=0; l_l<l_trace_size; l_l++)   // loop over samples of one trace 
        {
            /// disentangle data
           
            l_dat_fir[i][l_l] = *this->pdata++;
            l_dat_sec[i][l_l] = l_dat_fir[i][l_l];
            
            l_dat_fir[i][l_l] =  l_dat_fir[i][l_l] & 0x3fff;         // 14bit
            l_dat_sec[i][l_l] = (l_dat_sec[i][l_l] >> 16) & 0x3fff;
        }
            l_trace_size = l_trace_size * 2; 
            l_trace_trail = *this->pdata; 
       
//         if ( ((l_trace_trail & 0xf0000000) >> 24) != 0xb0)
//         {
//              printf ("ERROR>> trace trailer id is not 0xbb (0x%08x)\n",l_trace_trail);
//         }
 
                    } ///end if trace header
           fired_FEBEX_traces++;

                } /// Trace Channel loop
            }///Ge traces active 
        } ///  channel header condition
        
   
        if (num_modules != 0){
            this->pdata++; 
            FEBEXhead  = (FEBEX_Header*) this->pdata;
        }
        else FEBEX_data_loop = false; // Exits FEBEX Loop //
    }
}

//---------------------------------------------------------------

void Germanium_Detector_System::reset_fired_channels(){
    
    fired_FEBEX_amount = 0;
    num_channels_fired = 0;
    
    for(int i = 0;i < max_am_dets;++i){
        Sum_Time[i] = -1;
        pileup_flags[i] = -1;
        Ge_channels[i] = 0;
        Pileup[i] = -1;
        Hit_Pattern[i] = 0;
        Chan_Time[i] = 0;
        Chan_Energy[i] = 0;
        Chan_CF[i]=0;
        if(Germanium_TRACES_ACTIVE){
          for(int k=0; k<Germanium_TRACE_LENGTH/2; k++){
                        l_dat_fir[i][k]=0;
                        l_dat_sec[i][k]=0;
                    }
            }
    }
}
    
//---------------------------------------------------------------

// void Germanium_Detector_System::Calibrate_FEBEX(int id){
// 
// //     Sum_Time[id] = Germanium_T_CALIB->Calibrate_FEBEX_Sum_T(Sum_Time[id],id);
// // 
// //     Chan_Time[id] = Germanium_T_CALIB->Calibrate_FEBEX_Chan_T(Chan_Time[id],id);
//    // Chan_Energy[id] = Germanium_E_CALIB->Calibrate_FEBEX_E(Chan_Energy[id],id);
//    // cout << "2) Chan_Energy[id] " <<Chan_Energy[id] << " id " << id << endl;
// }

//---------------------------------------------------------------

int* Germanium_Detector_System::get_pdata(){return pdata;}

//---------------------------------------------------------------
