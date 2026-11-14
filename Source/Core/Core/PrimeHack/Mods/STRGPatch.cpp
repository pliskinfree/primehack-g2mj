#include "Core/PrimeHack/Mods/STRGPatch.h"

#include <cassert>

#include "Core/PowerPC/PowerPC.h"
#include "Core/PowerPC/MMU.h"
#include "Core/PrimeHack/GuestAllocator.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/System.h"

constexpr u32 STR_TABLE_SIZE = 0x1000;
const std::string CN_PREFIX = "\x01cn:";

namespace prime {
namespace {

u32 bsearch_strg_table(PowerPC::MMU& mmu, std::string const& key, u32 strg_header) {
  u32 bsearch_left = mmu.Read<u32>(strg_header + 0x14);
  int dist = mmu.Read<u32>(strg_header + 0x8);
  while (dist > 0) {
    int midpoint_offset = (dist * 4) & ~0x7;
    int half_dist = dist >> 1;
    std::string test_key = readin_str(mmu, mmu.Read<u32>(bsearch_left + midpoint_offset));
    if (test_key.compare(key) < 0) {
      dist -= (1 + half_dist);
      bsearch_left += midpoint_offset + 8;
    } else {
      dist = half_dist;
    }
  }
  return bsearch_left;
}

void patch_strg_entry_mp3_and_menu(PowerPC::PowerPCState& ppc_state, PowerPC::MMU& mmu, u32) {
  GetMod<STRGPatch>()->patch_strg_entry_vmc_common(
    ppc_state, mmu, ppc_state.gpr[3], ppc_state.gpr[4]);
  ppc_state.gpr[0] = ppc_state.spr[SPR_LR];
}

} // namespace

void STRGPatch::patch_strg_entry_vmc_common(PowerPC::PowerPCState& ppc_state, PowerPC::MMU& mmu, u32 strg_header, u32 key_ptr) {
  std::string key = readin_str(mmu, key_ptr);

  auto replacement = replace_tbl.find(key);
  if (replacement != replace_tbl.end()) {
    u32 bsearch_result = bsearch_strg_table(mmu, key, strg_header);
    std::string found_key = readin_str(mmu, mmu.Read<u32>(bsearch_result));
    if (found_key == key) {
      u32 strg_val_index = mmu.Read<u32>(bsearch_result + 4);
      u32 strg_val_table = mmu.Read<u32>(strg_header + 0x1c);
      // Chinese fan translation (text written into the ENGL table): if the game's own string
      // contains CJK (UTF-8 lead byte 0xE3-0xE9), use the Chinese replacement when one exists.
      u32 slot = replacement->second.first;
      auto cn = replace_tbl.find(CN_PREFIX + key);
      if (cn != replace_tbl.end()) {
        std::string orig = readin_str(mmu, mmu.Read<u32>(strg_val_table + 4 * strg_val_index));
        for (unsigned char c : orig) {
          if (c >= 0xE3 && c <= 0xE9) {
            slot = cn->second.first;
            break;
          }
        }
      }
      mmu.Write<u32>(slot + guest_table_addr, strg_val_table + 4 * strg_val_index);
    }
  }
}

void STRGPatch::run_mod(Game game, Region region) {
  switch (game) {
    case Game::MENU:
    case Game::PRIME_3_STANDALONE:
    case Game::PRIME_3:
      run_mod_common(guest_table_addr);
      break;

    case Game::PRIME_1:
    case Game::PRIME_1_GCN:
    case Game::PRIME_1_GCN_R1:
    case Game::PRIME_1_GCN_R2:
    case Game::PRIME_2:
    case Game::PRIME_2_GCN:
      break;

    default:
      break;
  }
}

bool STRGPatch::init_mod(Game game, Region region) {
  clear_table();

  switch (game) {
    case Game::MENU: {
      guest_table_addr = GuestAlloc(STR_TABLE_SIZE);

      add_table_entry("NunchukRequired", GetMotd());
      add_table_entry(CN_PREFIX + "NunchukRequired", "\xE6\x84\x9F\xE8\xB0\xA2\xE4\xBD\xBF\xE7\x94\xA8 PrimeHack\xEF\xBC\x81\n\xE5\xA6\x82\xE9\x9C\x80\xE5\xB8\xAE\xE5\x8A\xA9\xEF\xBC\x8C\xE8\xAF\xB7\xE6\x9F\xA5\xE9\x98\x85 PrimeHack \xE7\x9A\x84 wiki\xE3\x80\x82" /* 感谢使用 PrimeHack！\n如需帮助，请查阅 PrimeHack 的 wiki。 */);
      add_table_entry(CN_PREFIX + "DifficultyMenu_Easiest",
        "&link=[starteasiest]?typewrite=reverse;&wholepane;&rollover=menu2_hl;[ \xE6\x99\xAE\xE9\x80\x9A\xEF\xBC\x88\xE7\xAE\x80\xE5\x8D\x95\xEF\xBC\x89 ]&endlink;" /* &link=[starteasiest]?typewrite=reverse;&wholepane;&rollover=menu2_hl;[ 普通（简单） ]&endlink; */);
      add_table_entry(CN_PREFIX + "DifficultyMenu_Medium",
        "&link=[startmedium]?typewrite=reverse;&wholepane;&rollover=menu3_hl;[ \xE8\x80\x81\xE6\x89\x8B\xEF\xBC\x88\xE6\x99\xAE\xE9\x80\x9A\xEF\xBC\x89 ]&endlink;" /* &link=[startmedium]?typewrite=reverse;&wholepane;&rollover=menu3_hl;[ 老手（普通） ]&endlink; */);
      add_table_entry(CN_PREFIX + "DifficultyMenu_Hardest",
        "&if=HypermodeUnlocked;&link=[starthardest]?typewrite=reverse;&wholepane;&rollover=menu4_hl;[ \xE8\xB6\x85\xE8\x83\xBD\xEF\xBC\x88\xE5\x9B\xB0\xE9\x9A\xBE\xEF\xBC\x89 ]&endlink;&endif;" /* &if=HypermodeUnlocked;&link=[starthardest]?typewrite=reverse;&wholepane;&rollover=menu4_hl;[ 超能（困难） ]&endlink;&endif; */);
      add_table_entry("DifficultyMenu_Easiest",
        "&link=[starteasiest]?typewrite=reverse;&wholepane;&rollover=menu2_hl;[ Normal (Easy) ]&endlink;");
      add_table_entry("DifficultyMenu_Medium",
        "&link=[startmedium]?typewrite=reverse;&wholepane;&rollover=menu3_hl;[ Veteran (Normal) ]&endlink;");
      add_table_entry("DifficultyMenu_Hardest",
        "&if=HypermodeUnlocked;&link=[starthardest]?typewrite=reverse;&wholepane;&rollover=menu4_hl;[ Hypermode (Hard) ]&endlink;&endif;");
      int vmc_id = Core::System::GetInstance().GetPowerPC().RegisterVmcall(patch_strg_entry_mp3_and_menu);
      if (region == Region::NTSC_U) {
        add_code_change(0x8037e510, gen_vmcall(vmc_id, 0));
      } else if (region == Region::PAL) {
        add_code_change(0x8037e15c, gen_vmcall(vmc_id, 0));
      }
      break;
    }
    case Game::PRIME_1:
    case Game::PRIME_1_GCN:
    case Game::PRIME_1_GCN_R1:
    case Game::PRIME_1_GCN_R2:
    case Game::PRIME_2:
    case Game::PRIME_2_GCN:
      break;
    case Game::PRIME_3_STANDALONE: {
      guest_table_addr = GuestAlloc(STR_TABLE_SIZE);

      add_table_entry("NunchukRequired", GetMotd());
      add_table_entry(CN_PREFIX + "NunchukRequired", "\xE6\x84\x9F\xE8\xB0\xA2\xE4\xBD\xBF\xE7\x94\xA8 PrimeHack\xEF\xBC\x81\n\xE5\xA6\x82\xE9\x9C\x80\xE5\xB8\xAE\xE5\x8A\xA9\xEF\xBC\x8C\xE8\xAF\xB7\xE6\x9F\xA5\xE9\x98\x85 PrimeHack \xE7\x9A\x84 wiki\xE3\x80\x82" /* 感谢使用 PrimeHack！\n如需帮助，请查阅 PrimeHack 的 wiki。 */);
      add_table_entry(CN_PREFIX + "ShakeOffGandrayda",
                      "&just=center;\xE6\x8C\x89\xE4\xBD\x8F\xE8\xB7\xB3\xE8\xB7\x83\xE9\x94\xAE [&image=0x5FC17B1F30BAA7AE;] \xE6\x8C\xA3\xE8\x84\xB1\xE7\x94\x98\xE5\xBE\xB7\xE8\x95\xBE\xE8\xBE\xBE\xEF\xBC\x81" /* &just=center;按住跳跃键 [&image=0x5FC17B1F30BAA7AE;] 挣脱甘德蕾达！ */);
      add_table_entry("ShakeOffGandrayda",
                      "&just=center;Hold Jump [&image=0x5FC17B1F30BAA7AE;] to shake off Gandrayda!");
      int vmc_id = Core::System::GetInstance().GetPowerPC().RegisterVmcall(patch_strg_entry_mp3_and_menu);
      if (region == Region::NTSC_U) {
        add_code_change(0x803cdbd8, gen_vmcall(vmc_id, 0));
      } else if (region == Region::PAL) {
        add_code_change(0x803cf4b8, gen_vmcall(vmc_id, 0));
      }
      break;
    }
    case Game::PRIME_3: {
      guest_table_addr = GuestAlloc(STR_TABLE_SIZE);

      add_table_entry(CN_PREFIX + "ShakeOffGandrayda",
                      "&just=center;\xE6\x8C\x89\xE4\xBD\x8F\xE8\xB7\xB3\xE8\xB7\x83\xE9\x94\xAE [&image=0x5FC17B1F30BAA7AE;] \xE6\x8C\xA3\xE8\x84\xB1\xE7\x94\x98\xE5\xBE\xB7\xE8\x95\xBE\xE8\xBE\xBE\xEF\xBC\x81" /* &just=center;按住跳跃键 [&image=0x5FC17B1F30BAA7AE;] 挣脱甘德蕾达！ */);
      add_table_entry("ShakeOffGandrayda",
                      "&just=center;Hold Jump [&image=0x5FC17B1F30BAA7AE;] to shake off Gandrayda!");
      int vmc_id = Core::System::GetInstance().GetPowerPC().RegisterVmcall(patch_strg_entry_mp3_and_menu);
      if (region == Region::NTSC_U) {
        add_code_change(0x803cc268, gen_vmcall(vmc_id, 0));
      } else if (region == Region::PAL) {
        add_code_change(0x803cb984, gen_vmcall(vmc_id, 0));
      }
      break;
    }
    default:
      break;
  }
  return true;
}

void STRGPatch::add_table_entry(std::string key, std::string val) {
  if (key.empty()) {
    return;
  }

  u32 val_len = static_cast<u32>(val.length());
  if (replace_tbl.count(key) > 0) {
    replace_tbl[key].second = val;
    recompute_tbl_off();
  } else if (current_tbl_off + val_len + 1 >= STR_TABLE_SIZE) {
    assert("STRGPatch: string pool is out of capacity.");
    return;
  } else {
    replace_tbl[key] = std::make_pair(current_tbl_off, val);
    current_tbl_off += val_len + 1;
  }
}

void STRGPatch::recompute_tbl_off() {
  current_tbl_off = 0;
  for (auto& [k, vp] : replace_tbl) {
    vp.first = current_tbl_off;
    current_tbl_off += static_cast<u32>(vp.second.length()) + 1;
  }
}

void STRGPatch::clear_table() {
  current_tbl_off = 0;
  replace_tbl.clear();
}

void STRGPatch::run_mod_common(u32 tbl_address) {
  for (auto const& [_, repl_pair] : replace_tbl) {
    for (u32 i = 0; i < static_cast<u32>(repl_pair.second.length()); i++) {
      write8(repl_pair.second[i], tbl_address + repl_pair.first + i);
    }
    write8(0, tbl_address + repl_pair.first + static_cast<u32>(repl_pair.second.length()));
  }
}

} // namespace prime
