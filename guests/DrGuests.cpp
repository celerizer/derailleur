#include "DrGuests.h"
#include "../DrGuestList.h"

#include "impl/BanjoTooie.h"
#include "impl/DiddyKongRacing.h"
#include "impl/Kirby64.h"
#include "impl/MarioGolf.h"
#include "impl/MarioKart64.h"
#include "impl/MarioKartDoubleDash.h"
#include "impl/MarioKartWii.h"
#include "impl/MarioParty1.h"
#include "impl/MarioParty2.h"
#include "impl/MarioParty3.h"
#include "impl/MarioParty4.h"
#include "impl/MarioParty5.h"
#include "impl/MarioParty6.h"
#include "impl/MarioParty7.h"
#include "impl/MarioParty8.h"
#include "impl/MarioParty9.h"
#include "impl/MarioPartyAdvance.h"
#include "impl/MarioTennis.h"
#include "impl/MaryKateAndAshleySweet16LicensedToDrive.h"
#include "impl/PokemonStadium.h"
#include "impl/PokemonStadium2.h"
#include "impl/SmashRemix.h"
#include "impl/SonicShuffle.h"
#include "impl/StarFox64.h"
#include "impl/SuperMarioBros3.h"
#include "impl/WiiParty.h"
#include "impl/YoshisIsland.h"

static void dr_add(DrGuestList *list, DrGuest *guest)
{
  if (guest->isValid())
    list->add(guest);
  else
    delete guest;
}

void dr_create_guests(DrGuestList *list)
{
  dr_add(list, new BanjoTooie());
  dr_add(list, new DiddyKongRacing());

  dr_add(list, new Kirby64());

  dr_add(list, new MarioGolf());
  dr_add(list, new MarioKart64());
  dr_add(list, new MarioKartDoubleDash());
  dr_add(list, new MarioKartWii());
  dr_add(list, new MarioParty1());
  dr_add(list, new MarioParty2());
  dr_add(list, new MarioParty3());
  dr_add(list, new MarioParty4());
  dr_add(list, new MarioParty5());
  dr_add(list, new MarioParty6());
  dr_add(list, new MarioParty7());
  dr_add(list, new MarioParty8());
  dr_add(list, new MarioParty9());
  dr_add(list, new MarioPartyAdvance());
  dr_add(list, new MarioTennis());
  dr_add(list, new MaryKateAndAshleySweet16LicensedToDrive());

  dr_add(list, new PokemonStadium());
  dr_add(list, new PokemonStadium2());

  dr_add(list, new SmashRemix());
  dr_add(list, new SonicShuffle());
  dr_add(list, new StarFox64());
  dr_add(list, new SuperMarioBros3());

  dr_add(list, new WiiParty());

  dr_add(list, new YoshisIsland());
}
