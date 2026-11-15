#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <ostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include "ftxui/dom/elements.hpp"
#include "ftxui/ftxui.hpp"

using namespace ftxui;
using namespace nlohmann;
using namespace cpr;

json config_o, config_i, profile_data,items_js;

enum State{login = 0, tutorial = 1, tracker = 2, error = 3};

int state = login;
int pre_state = login;

std::string buffer;
std::string AccountID_Input;
std::string login_MSG = "";
std::string Error_MSG;
std::string cpr_MSG ="";
int status_code;

bool is_valid(const std::string &str){
if(!str.empty() && std::ranges::all_of(str, [](unsigned char c){return std::isxdigit(c);})){
  if(str.size() == 24){
    return true;
  }
  else{
    login_MSG = "Input does not equal 24 digits";
    return false;
  }
}
else{
  login_MSG = "Input was empty or contain a non-hex character ";
  return false;
}
}

class profile_checker{
  public:
    std::string Accountname;
    std::string update_MSG;

    json profile_checker_data;
    std::unordered_map<std::string, int64_t> player_xp;

    bool failed_profile_update = false;
    bool failed_items_update = false;

   void profilechecker(){
     std::string accountid;
     config_i = nullptr;
      if(std::filesystem::exists("profile_new_new.json")){
        grab_profile_data();
        state = tracker;
      }
      else{
        if(std::filesystem::exists("config.json")){

            std::ifstream file("config.json");
            if(!file){
              Error_MSG = "Error : Could not open config.json";
              state = error;
              failed_profile_update = true;
              return;
            }
            file >> config_i;
            file.close();
            if(!file){
              Error_MSG = "Error: Cannot read config.json";
              state = error;
              failed_profile_update = true;
              return;
            }

            accountid = config_i["AccountID"];
          
          Response grab_profile = Get(Url{"https://api.warframe.com/cdn/getProfileViewingData.php"}, Parameters{{"playerId",accountid}});
          if(grab_profile.status_code != 200){
            Error_MSG = "Error: Failed to request profile data "  "Status code: " + std::to_string(grab_profile.status_code);
            state = error;
            failed_profile_update = true;
            return;
          }
          else{
            if(!std::filesystem::exists("profile_new_new.json")){
              try{
                config_o = json::parse(grab_profile.text);
              }
              catch(const json::parse_error& e){
                Error_MSG = "Error: Failed to parse profile data request";
                state = error;
                failed_profile_update = true;
                return;
              }

            std::ofstream file("profile_new_new.json");
            if(!file){
              Error_MSG = "Error: Cannot open profile_new_new.json";
              state = error;
              failed_profile_update = true;
              return;
            }
            file << config_o;
            file.close();
            if(!file){
              Error_MSG = "Error: Cannot write to profile_new_new.json";
              state = error;
              failed_profile_update = true;
              return;
            }
            grab_profile_data();
            if(std::filesystem::exists("profile_new_new_tmp.json")){
              try{
              std::filesystem::remove("profile_new_new_tmp.json");
              }
              catch(const std::filesystem::filesystem_error& e){
                Error_MSG = "Error: Failed to delete profile_new_new_tmp.json. Please Delete it";
                state = error;
              }
            }
            update_MSG = "";
           state = tracker;
            }
          }

        }
        else{
        state = login;
        }
      }
    }
   
   void grab_profile_data(){
     player_xp.clear();
        std::ifstream file("profile_new_new.json");
        if(!file){
          Error_MSG = "Error: Could not open profile_new_new.json.";
          state = error;
          return;
        }
        file >> profile_checker_data;
        if(!file){
          Error_MSG = "Error: Could not read profile_new_new.json";
          state = error;
          return;
        }
        Accountname = profile_checker_data["Results"][0]["DisplayName"];
        Accountname.erase(Accountname.end() - 1);

        auto player_items = profile_checker_data["Results"][0]["LoadOutInventory"]["XPInfo"];
        int player_items_size = profile_checker_data["Results"][0]["LoadOutInventory"]["XPInfo"].size();

        for(size_t t = 0; t < player_items_size; ++t){
          std::string PI = player_items[t]["ItemType"];
          int XP = player_items[t]["XP"];

          player_xp[PI] = XP;
        }
   }

   void items_data_checker(){
     if(!std::filesystem::exists("items.json")){

       Response item_R = Get(Url{"https://api.warframestat.us/items"});
       if(item_R.status_code != 200){
         Error_MSG = "Error: items request failed. status= " + std::to_string(item_R.status_code) + "  error = " + item_R.error.message + '\n';
         state = error;
         failed_items_update = true;
         return;
       }

         std::ofstream file("items.json.tmp", std::ios::binary);
         if(!file){
           Error_MSG = "Error: Cannot open items_tmp.json\n";
           state = error;
           failed_items_update = false;
           return;
         }
         file << item_R.text;
         file.close();
         if(!file){
           Error_MSG = "Error: Cannot write to items_tmp.json\n";
           state = error;
           failed_items_update = true;
           return;
         }

         try{
         std::filesystem::rename("items.json.tmp", "items.json");
         }
         catch(const std::filesystem::filesystem_error& e){
           Error_MSG = "Error: Failed to rename items.json.tmp. Please delete it";
           state = error;
           failed_items_update = true;
         }
     }
   }

   void load_items(){
     items_data_checker();
     if(std::filesystem::exists("items.json")){

       std::ifstream file("items.json");
       if(!file){
         Error_MSG = "Error: load_item() cannot load items.json";
         state = error;
         failed_items_update = true;
       }
       else{
         try{
            file >> items_js;
            file.close();
         }
         catch(const json::parse_error& e){
           Error_MSG = "Error: load_items() failed to parse items.json";
           items_js = nullptr;
           failed_items_update = true;
           state = error;
           try{
           std::filesystem::remove("items.json");
           items_data_checker();
            }
           catch(const std::filesystem::filesystem_error& e){
             Error_MSG = "items.json was corrupt and has been re-downloaded. Please restart.";
             state = error;
             failed_items_update = true;
             return;
           }
         }
       }
     }
   }

   void update_profile(){
     failed_profile_update = false;
     if(std::filesystem::exists("profile_new_new.json")){
       try{
         std::filesystem::rename("profile_new_new.json", "profile_new_new_tmp.json");
       }
       catch(const std::filesystem::filesystem_error& e){
         Error_MSG = "Error: Failed to rename while resetting data";
         state = error;
         return;
       }

       profilechecker();
       if(failed_profile_update == true){
         try{
         std::filesystem::rename("profile_new_new_tmp.json", "profile_new_new.json");
         }
         catch(const std::filesystem::filesystem_error& e){
           Error_MSG = "Error: Failed to rename the temporary profile to normal";
           state = error;
           update_MSG = "Failed to update profile";
           return;
         }
         grab_profile_data();
         failed_profile_update = false;
         update_MSG = "Failed to update profile";
         return;
       }
         auto now = std::chrono::system_clock::now();
         auto floored_now = std::chrono::floor<std::chrono::seconds>(now);
         update_MSG = "profile: " + std::format("{:%m-%d-%Y %H:%M:%S}", floored_now);
       
     }
     else{
       profilechecker();
       if(failed_profile_update){
       update_MSG = "Failed to update profile";
       }
       else{
         auto now = std::chrono::system_clock::now();
         auto floored_now = std::chrono::system_clock::now();
         update_MSG = "profile: " + std::format("{:%m-%d-%Y %H:%M:%S}", floored_now);
       }
     }
   }

   void update_item(){
     failed_items_update = false;
     update_MSG = "Updating items";
     if(std::filesystem::exists("items.json")){
       try{
         std::filesystem::rename("items.json", "items_tmp.json");
       }
       catch(const std::filesystem::filesystem_error& e){
         Error_MSG = "Error: failed to rename items.json";
         state = error;
         return;
       }

       items_data_checker();
       if(!failed_items_update){
         load_items();
       }
       if(failed_items_update == true){
         try{
           std::filesystem::remove("items.json");
           std::rename("items_tmp.json", "items.json");
         }
         catch(const std::filesystem::filesystem_error& e){
           Error_MSG = "Error: rename failed on items_tmp.json";
           state = error;
           return;
         }
         load_items();
         failed_items_update = false;
         update_MSG = "failed to update items";
         return;
       }

       std::filesystem::remove("items_tmp.json");
     }
     else{
       load_items();
       if(failed_items_update){
       update_MSG = "No items.json Grabbing items_data";
       return;
       }
     }

     auto now = std::chrono::system_clock::now();
     auto floored_now = std::chrono::floor<std::chrono::seconds>(now);
     update_MSG = "items:" + std::format("{:%m-%d-%Y %H:%M:%S}",floored_now);
   }
};

profile_checker pc;

Component login_screen(ScreenInteractive &screen, int &state){
  int pre_state = login;
  bool focused = true;

  InputOption opt;
  opt.multiline = false;
  opt.transform = [](InputState s){
    s.element |= bgcolor(Color::Black) | color(Color::White);
    if(s.is_placeholder) s.element |= dim;
    return s.element;
  };
  opt.on_enter = [&]{

    AccountID_Input = buffer;

    if(!is_valid(AccountID_Input)){
      AccountID_Input.clear();
      buffer.clear();
      return;
    }

      config_o["AccountID"] = AccountID_Input;

        std::ofstream file("config.json");

        if(!file){
          login_MSG = "Error: Could not create config.json";
          return;
        }

        file << config_o.dump(2);

        if(!file){
          login_MSG = "Error: Could not write to config.json";
          return;
        }
      
        login_MSG = "Created config file";
        
        Response grab_profile = Get(Url{"https://api.warframe.com/cdn/getProfileViewingData.php"}, Parameters{{"playerId",AccountID_Input}});

       status_code = grab_profile.status_code;

       if(status_code == 200){
         try{
           profile_data = json::parse(grab_profile.text);
         }
         catch(const json::parse_error& e){
           cpr_MSG = "Error: json_parse failed";
           return;
         }

         std::ofstream write_profile("profile_new_new.json");

         if(!write_profile){
           cpr_MSG = "Error: Could not create profile_new_new.json";
           return;
         }

         write_profile << profile_data;

         write_profile.close();

         if(write_profile){
         cpr_MSG = "Account data was found!";
           state = tracker;
         }

       }
       else if(status_code == 403){
         cpr_MSG = "AccountID url was blocked";
       }
       else if(status_code == 409){
         cpr_MSG = "AccountID url was not recognized";
       }
       else if(status_code == 404){
         cpr_MSG = "AccountID url failed to connect";
       }
  };
  auto cin = Input(&buffer, "", opt);

  auto button = Button("How to get AccountID", [&state,cin]{
      cin->TakeFocus();
      state = tutorial;
      });

  auto widget = Container::Horizontal({cin, button});

  return Renderer(widget, [=]{
      return vbox({
          filler(),
          text("Enter your Warframe AccountID") | hcenter,
          cin->Render() | size(WIDTH, EQUAL, 24) | hcenter | border,
          text(login_MSG),
          text(std::to_string(status_code)),
          text(cpr_MSG),
          filler(),
          hbox({
              filler(),
              button->Render(),
              }),
          }) | size(WIDTH, EQUAL, 60) | size(HEIGHT, EQUAL, 20) | border | center;
      });

}

Component accountID_tutorial_screen(ScreenInteractive &screen, int &state){
  
  auto back_btn = Button("Back", [&]{
      state = login;
      });

  return Renderer(back_btn, [=]{
      return vbox({
          text("tutorial_Screen"),
          text("Step 1: Login to the warframe website: https://www.warframe.com/en"),
          text(""),
          text("Step 2: Inspect the website code with F12 to go into Devtool/inspector."),
          text("Google chrome: press F12"),
          text("Safari: press OPTION + COMMAND + I"),
          text(""),
          paragraph("Step 3: Go into the Storage tab(Safari) or Application tab(Google Chrome) and click on Cookies and Warframe.com and there will be a cookie called gid and it has a value that is your AccountID"),
          text(""),
          text("Step 4: Copy the gid value and paste inside the input box and press Enter ONCE and wait for what the resoonse is"),
          text(""),
          text("Step 5: If stil confused, ask AI how to grab your warframe accountID"),
          text(""),
          text(""),
          back_btn->Render() | size(WIDTH, EQUAL, 20),
          });
      });
}

//filter variables
  bool not_mastered_filter = false;
  bool is_mastered_filter = true;
  bool is_weapon = false;

  std::vector<std::string> category = {"Warframes", "Weapons", "Companions"};
  std::vector<std::string> warframe_type = {"All", "Warframe", "Archwing"};
  std::vector<std::string> weapon_type = {"All", "Primary", "Secondary", "Melee", "Arch-Gun", "Arch-Melee"};
  std::vector<std::string> companion_type = {"All", "Pets", "Sentinels"};
  std::vector<std::string> Master_rank = {"All","0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17"};
  std::vector<std::string> item_name_vec;
  std::vector<int> item_xp_vec;

  int wt_selected = 0;
  int warframe_type_selected = 0;
  int companion_type_selected = 0;
  int cat_selected = 0;
  int item_name_selected = 0;
  int item_count = 0;
  int MR_selected = 0;
   
  std::string category_str;
  std::string weapon_type_str;
  std::string warframe_type_str;
  std::string companion_type_str;
  std::string filter_name = "Is_Mastered";
  std::string item_name;
  std::string item_xp;

void build_item_list(){
  item_name_vec.clear();
  item_xp_vec.clear();
  item_count = 0;

  switch(warframe_type_selected){
    case 0:
      warframe_type_str = "All";
      break;
    case 1:
      warframe_type_str = "Warframe";
      break;
    case 2:
      warframe_type_str = "Archwing";
      break;

  }

    switch(wt_selected){
    case 0:
      weapon_type_str = weapon_type[0];
      break;
    case 1:
      weapon_type_str = weapon_type[1];
      break;
    case 2:
      weapon_type_str = weapon_type[2];
      break;
    case 3:
      weapon_type_str = weapon_type[3];
      break;
    case 4: 
      weapon_type_str = weapon_type[4];
      break;
    case 5:
      weapon_type_str = weapon_type[5];
      break;
  }

    switch(companion_type_selected){
      case 0:
        companion_type_str = companion_type[0];
        break;
      case 1:
        companion_type_str = companion_type[1];
        break;
      case 2:
        companion_type_str = companion_type[2];
        break;
    }

    for(const auto& item : items_js){
      if(!item.value("masterable", false)) continue;

      std::string cat = item.value("category","");
      std::string type = item.value("type","");
      std::string name = item.value("name", "");
      std::string un = item.value("uniqueName", "");
      int mr = item.value("masteryReq",0);
      std::string mr_str = std::to_string(mr);
      int max_cap = item.value("maxLevelCap", 30);
      auto found = pc.player_xp.find(un);
      int64_t xp = 0;
      if(found != pc.player_xp.end()){
        xp = found->second;
      }

      if(cat_selected == 0){
        is_weapon = false;
        if(is_mastered_filter == true){
        if(found != pc.player_xp.end()){
        if(warframe_type_str == warframe_type[1]){
          if(cat == "Warframes"){
            if(max_cap == 30){
            if(found->second >= 900000){
              item_name_vec.push_back(name);
              item_count++;
            }
            }
            else{
              if (found->second >= 1600000){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
          }
        }
          if(warframe_type_str == warframe_type[2]){
           if (cat == "Archwing"){
             if(found->second >= 900000){
               item_name_vec.push_back(name);
               item_count++;
             }
          }
          }
          if(warframe_type_str == warframe_type[0]){
            if(cat == "Archwing" || cat == "Warframes"){
              if(max_cap == 30){
              if(found->second >= 900000){
                item_name_vec.push_back(name);
                item_count++;
              }
              }
              else{
                if(found->second >= 1600000){
                  item_name_vec.push_back(name);
                item_count++;
                }
              }
            }
          }
      }
      }
        if(not_mastered_filter == true){
          if(found != pc.player_xp.end()){
            if(warframe_type_str == warframe_type[1]){
              if(cat == "Warframes"){
                if(max_cap == 30){
                if(found->second < 900000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
                }
                else{
                  if(found->second < 1600000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
              }
            }
            if(warframe_type_str == warframe_type[2]){
              if(cat == "Archwing"){
                if(found->second < 900000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
            }
            if(warframe_type_str == warframe_type[0]){
              if(cat == "Warframes" || cat == "Archwing"){
                if(max_cap == 30){
                if(found->second < 900000){
                  item_name_vec.push_back(name);
                    item_count++;
                }
                }
                else{
                  if(found->second < 1600000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
              }
            }
          }
          if(found == pc.player_xp.end()){
            if(warframe_type_str == warframe_type[1]){
              if(cat == "Warframes"){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
            if(warframe_type_str == warframe_type[2]){
              if(cat == "Archwing"){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
            if(warframe_type_str == warframe_type[0]){
              if(cat == "Warframes" || cat == "Archwing"){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
          }
        }
      }

      if(cat_selected == 1){
        is_weapon = true;
        if(is_mastered_filter == true){
          if(found != pc.player_xp.end()){
          if(weapon_type_str== weapon_type[0]){
            if(MR_selected == 0){
      if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
        if(max_cap == 30){
          if(found->second >= 450000){
            item_name_vec.push_back(name);
            item_count++;
          }
        }
        else{
          if(found->second >= 800000){
            item_name_vec.push_back(name);
            item_count++;
          }
        }
      }
          }
            if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
                    if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
        if(max_cap == 30){
          if(found->second >= 450000){
            item_name_vec.push_back(name);
            item_count++;
          }
        }
        else{
          if(found->second >= 800000){
            item_name_vec.push_back(name);
            item_count++;
          }
        }
      }
            }
          }
          if(cat == weapon_type_str && weapon_type_str != weapon_type[0]){
            if(MR_selected == 0){
            if(max_cap == 30){
              if(found->second >= 450000){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
            else{
              if(found->second >= 800000){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
          }
            if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
              if(max_cap == 30){
                if(found->second >= 450000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
              else{
                if(found->second >= 800000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
            }
          }
          }
        }
        if(not_mastered_filter == true){
          if(found != pc.player_xp.end()){
            if(weapon_type_str == weapon_type[0]){
              if(MR_selected == 0){
              if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
                if(max_cap == 30){
                  if(found->second < 450000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
                else{
                  if(found->second < 800000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
              }
            }
              if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
                if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
                if(max_cap == 30){
                  if(found->second < 450000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
                else{
                  if(found->second < 800000){
                    item_name_vec.push_back(name);
                    item_count++;
                  }
                }
              }
              }
            }
            if(cat == weapon_type_str && weapon_type_str != weapon_type[0]){
              if(MR_selected == 0){
              if(max_cap == 30){
                if(found->second < 450000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
              else{
                if(found->second < 800000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
            }
              if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
                if(max_cap == 30){
                if(found->second < 450000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
                else{
                if(found->second < 800000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
              }
            }
          }
          if(found == pc.player_xp.end()){
            if(weapon_type_str == weapon_type[0]){
              if(MR_selected == 0){
              if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
              if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
               if(cat == "Primary" || cat == "Secondary" || cat == "Melee" || cat == "Arch-Gun" || cat == "Arch-Melee"){
                item_name_vec.push_back(name);
                item_count++;
              }
              }
            }

            if(cat == weapon_type_str && weapon_type_str != weapon_type[0]){
              if(MR_selected == 0){
              item_name_vec.push_back(name);
              item_count++;
              }
              if(mr_str == Master_rank[MR_selected] && MR_selected != 0){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
            }
        }
      }
      if(cat_selected == 2){
        is_weapon = false;
        if(is_mastered_filter ==true){
       if(found != pc.player_xp.end()){
         if(companion_type_str == companion_type[0]){
       if (cat == "Pets" || cat == "Sentinels"){
         if(found->second >= 900000){
           item_name_vec.push_back(name);
           item_count++;
         }
      }
         }
         if(cat == companion_type_str && companion_type_str != companion_type[0]){
           if(found->second >= 900000){
             item_name_vec.push_back(name);
             item_count++;
           }
         }
      }
        }
        if(not_mastered_filter == true){
          if(found != pc.player_xp.end()){
            if(companion_type_str == companion_type[0]){
              if(cat == "Pets" || cat == "Sentinels"){
                if(found->second < 900000){
                  item_name_vec.push_back(name);
                  item_count++;
                }
              }
            }
            if(cat == companion_type_str && companion_type_str != companion_type[0]){
              if(found->second < 900000){
                item_name_vec.push_back(name);
                item_count++;
              }
            }
          }
          if(found == pc.player_xp.end()){
            if(companion_type_str == companion_type[0]){
              if(cat == "Pets" || cat == "Sentinels"){
              item_name_vec.push_back(name);
              item_count++;
              }
            }
            if(cat == companion_type_str && companion_type_str != companion_type[0]){
              item_name_vec.push_back(name);
              item_count++;
            }
          }
        }
      }

        while(item_xp_vec.size() < item_name_vec.size()){
          item_xp_vec.push_back(xp);
        }
      
    }

    if(item_name_vec.empty()){
      item_name = "";
      item_name_selected = 0;
      item_name_selected = 0;
    }
    else{
      if(item_name_selected >= (int)item_name_vec.size()){
        item_name_selected = item_name_vec.size() - 1;
      }
    item_name = item_name_vec[item_name_selected];
    item_xp = std::to_string(item_xp_vec[item_name_selected]);
    }
}


Component tracker_screen(ScreenInteractive &screen, int &state){
  int pre_state = tracker;
  std::string Warframe_username = pc.Accountname;


  //Components section==============================================================================================
  auto category_dropbox = Dropdown(&category, &cat_selected);

  auto warframe_type_dropbox = Dropdown(&warframe_type, &warframe_type_selected);

  auto weapon_type_dropbox = Dropdown(&weapon_type, &wt_selected);

  auto companions_type_dropbox = Dropdown(&companion_type, &companion_type_selected);

  auto master_rank_dropbox = Dropdown(&Master_rank, &MR_selected);

  auto master_rank_dropbox_maybe = Maybe(master_rank_dropbox, &is_weapon);

  auto Not_Mastered_filter_btn = Button("Not Mastered", [&]{
      filter_name = "Not Mastered";
      not_mastered_filter = true;
      is_mastered_filter = false;
      });

  auto Is_Mastered_filter_btn = Button("Is Mastered", [&]{
      filter_name = "Is Mastered";
      is_mastered_filter = true;
      not_mastered_filter = false;
      });

  auto update_profile_btn = Button("profile", [&]{
      pc.update_MSG = "";
      pc.update_profile();
      });

  auto update_items_btn = Button("items", [&]{
      pc.update_MSG = "Updating";
      pc.update_item();
  });

  auto item_list = Menu(&item_name_vec, &item_name_selected);

  auto filter_switch = Container::Tab({warframe_type_dropbox, weapon_type_dropbox, companions_type_dropbox}, &cat_selected);

  auto widget = Container::Vertical({category_dropbox, filter_switch, Not_Mastered_filter_btn, Is_Mastered_filter_btn, item_list, master_rank_dropbox_maybe,update_profile_btn, update_items_btn});


  //=======================================================================================================
  return Renderer(widget, [=]{
      build_item_list();
      return vbox({
          text(Warframe_username)| hcenter,
      hbox({
          vbox({
          text("Category") | hcenter,
              category_dropbox->Render() | hcenter,
              separator(),
              text("Type") | hcenter,
              filter_switch->Render() | hcenter,
              master_rank_dropbox_maybe->Render(),
              separator(),
              Not_Mastered_filter_btn->Render(),
              separator(),
              Is_Mastered_filter_btn->Render(), separator(),
              text(filter_name) | hcenter,
              separator(),
              text("Item Counts") | hcenter,
              text(std::to_string(item_count)) | border | hcenter,
              separator(),
              filler(),
              separator(),
              text("Update") | hcenter,
              text(pc.update_MSG),
              update_profile_btn->Render(),
              update_items_btn->Render(),
              }),
          separator(),
          hbox({
              vbox({
                  item_list->Render() | vscroll_indicator | frame | flex,
              }),
              vbox({
              filler(),
              text("Item Name") | hcenter ,
              text(item_name)  | border | size(WIDTH,EQUAL,30) | hcenter,
              text("Item XP") | hcenter,
              text(item_xp) | border | size(WIDTH, EQUAL, 30) | hcenter,
              filler(),
              filler(),
              filler(),
              }) | flex,
              }) | flex,
          }) | border | flex,
          });
      });
}

Component error_screen(ScreenInteractive &screen, int &state){
  auto back_btn = Button("Back",[&]{
      if(std::filesystem::exists("profile_new_new.json")){
      state = tracker;
      }
      else{
      state = login;
      }
      });

  return Renderer(back_btn,[=]{
      return vbox({
          text(Error_MSG),
          back_btn->Render(),
          }) | border | center;
      });
}

int main(){
  pc.profilechecker();
  pc.load_items();

  auto screen = ScreenInteractive::Fullscreen();

  auto Login_Screen = login_screen(screen, state);
  auto Account_Tutorial_Screen = accountID_tutorial_screen(screen, state);
  auto Tracker_Screen = tracker_screen(screen, state);
  auto Error_Screen = error_screen(screen,state);

  auto screenstate = Container::Tab({Login_Screen,Account_Tutorial_Screen, Tracker_Screen, Error_Screen}, &state);

  screen.Loop(screenstate);
}
