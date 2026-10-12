#include "common.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

typedef struct {
#ifdef VERSION_EU
    /* 0x0 */ u8 *unk0[3];            /* eu: English, German, French (NULL) */
    /* 0xC */ u16 *unk4;              /* eu: NULL */
#else
    /* 0x0 */ u8 *unk0;
    /* 0x4 */ u16 *unk4;
#endif
} UnkStruct_802FF188; /* size = 0x8, eu 0x10 */

extern u8 D_8039CAB6;

/* The second line of each entry, in the data after gzip's. */
extern u16 D_80303B9C[];
extern u16 D_80303BB4[];
extern u16 D_80303BC8[];
extern u16 D_80303BCC[];
extern u16 D_80303BD0[];
extern u16 D_80303BE4[];
extern u16 D_80303BF8[];
extern u16 D_80303C14[];
extern u16 D_80303C18[];
extern u16 D_80303C2C[];
extern u16 D_80303C48[];
extern u16 D_80303C4C[];
extern u16 D_80303C50[];
extern u16 D_80303C5C[];
extern u16 D_80303C70[];
extern u16 D_80303C74[];
extern u16 D_80303C78[];
extern u16 D_80303C88[];
extern u16 D_80303C98[];
extern u16 D_80303CAC[];
extern u16 D_80303CB0[];
extern u16 D_80303CC0[];
extern u16 D_80303CD8[];
extern u16 D_80303CF0[];
extern u16 D_80303CF4[];
extern u16 D_80303D0C[];
extern u16 D_80303D28[];
extern u16 D_80303D40[];
extern u16 D_80303D44[];
extern u16 D_80303D58[];
extern u16 D_80303D70[];
extern u16 D_80303D80[];
extern u16 D_80303D98[];
extern u16 D_80303DA4[];
extern u16 D_80303DA8[];
extern u16 D_80303DAC[];
extern u16 D_80303DC0[];
extern u16 D_80303DDC[];
extern u16 D_80303DE0[];
extern u16 D_80303DE4[];
extern u16 D_80303DFC[];
extern u16 D_80303E14[];
extern u16 D_80303E18[];
extern u16 D_80303E30[];
extern u16 D_80303E48[];
extern u16 D_80303E60[];
extern u16 D_80303E70[];
extern u16 D_80303E84[];
extern u16 D_80303E88[];
extern u16 D_80303E8C[];
extern u16 D_80303EA0[];
extern u16 D_80303EB8[];
extern u16 D_80303EBC[];
extern u16 D_80303ED0[];
extern u16 D_80303EE4[];
extern u16 D_80303EF4[];
extern u16 D_80303EF8[];
extern u16 D_80303F14[];
extern u16 D_80303F18[];
extern u16 D_80303F1C[];
extern u16 D_80303F20[];
extern u16 D_80303F24[];
extern u16 D_80303F28[];
extern u16 D_80303F34[];
extern u16 D_80303F4C[];
extern u16 D_80303F50[];
extern u16 D_80303F54[];
extern u16 D_80303F6C[];
extern u16 D_80303F84[];
extern u16 D_80303F88[];
extern u16 D_80303F8C[];
extern u16 D_80303F9C[];
extern u16 D_80303FB0[];
extern u16 D_80303FB4[];
extern u16 D_80303FB8[];
extern u16 D_80303FD4[];
extern u16 D_80303FF0[];
extern u16 D_80303FF4[];
extern u16 D_80303FF8[];
extern u16 D_80303FFC[];
extern u16 D_80304000[];
extern u16 D_80304004[];
extern u16 D_8030401C[];
extern u16 D_8030402C[];
extern u16 D_80304030[];
extern u16 D_80304034[];
extern u16 D_8030404C[];
extern u16 D_80304064[];
extern u16 D_80304080[];
extern u16 D_80304084[];
extern u16 D_80304094[];
extern u16 D_803040A4[];
extern u16 D_803040A8[];
extern u16 D_803040AC[];
extern u16 D_803040B0[];
extern u16 D_803040B4[];
extern u16 D_803040CC[];
extern u16 D_803040DC[];
extern u16 D_803040E0[];
extern u16 D_803040E4[];
extern u16 D_803040F4[];
extern u16 D_80304108[];
extern u16 D_80304118[];
extern u16 D_8030411C[];
extern u16 D_80304138[];
extern u16 D_80304154[];
extern u16 D_80304164[];
extern u16 D_80304168[];
extern u16 D_8030417C[];
extern u16 D_80304190[];
extern u16 D_803041A8[];
extern u16 D_803041AC[];
extern u16 D_803041B0[];
extern u16 D_803041B4[];

u8 D_802FF180[6] = { 0x04, 0x0A, 0x0D, 0x21, 0x0E, 0x11 };
/* A text of these is written to: func_802979E0 puts the count of those
   still to find in place of its digit.  The port has them as arrays,
   which may be written, not string literals, which may not be (the N64's
   are in RDRAM). */
#ifdef TARGET_PC
#define WTEXT(s) ((u8 *)(char[]){ s })
#else
#define WTEXT(s) (u8 *)s
#endif
UnkStruct_802FF188 D_802FF188[7][20] = {
#ifdef VERSION_EU
    {
        { { WTEXT("WELL, IT'S ABOUT TIME!"), WTEXT("HEY, WIRD ABER AUCH"), NULL }, NULL },
        { { WTEXT("DOES IT LOOK LIKE I'M"), WTEXT("ZEIT, DASS MICH JEMAND"), NULL }, NULL },
        { { WTEXT("ENJOYING MYSELF HERE?"), WTEXT("HIER HERAUSHOLT!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("AND YOU'VE STILL GOT"), WTEXT("NOCH SIND GENAU"), NULL }, NULL },
        { { WTEXT("0 OF THE OTHERS"), WTEXT("0 MEINER KOLLEGEN"), NULL }, NULL },
        { { WTEXT("LEFT TO TRACK DOWN."), WTEXT("NICHT GERETTET!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("VISIT GLORY CROSSING -"), WTEXT("SCHAU IN DER KATENA-"), NULL }, NULL },
        { { WTEXT("THERE'S BOUND TO BE"), WTEXT("SCHLUCHT NACH. DORT"), NULL }, NULL },
        { { WTEXT("ONE HOLED UP THERE."), WTEXT("IST EIN GEFANGENER."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("IT SHOULDN'T TAKE YOU"), WTEXT("BEEILE DICH, IHN"), NULL }, NULL },
        { { WTEXT("TOO LONG TO FIND HIM."), WTEXT("ZU FINDEN!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("THIS IS IT, I TELL YOU!"), WTEXT("DAS IST DAS ENDE, WIR"), NULL }, NULL },
        { { WTEXT("THE END! WE'RE ALL"), WTEXT("WERDEN ALLE..."), NULL }, NULL },
        { { WTEXT("GOING TO... OH."), WTEXT("OH MEIN..."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("WHAT ABOUT THE REST"), WTEXT("WAS IST MIT DEN"), NULL }, NULL },
        { { WTEXT("OF MY FRIENDS?"), WTEXT("ANDEREN? DU MUSST"), NULL }, NULL },
        { { WTEXT("ONLY 0 MORE LEFT!"), WTEXT("SIE RETTEN!!!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("WANT TO MAKE AN"), WTEXT("MACHE EINEN STOP"), NULL }, NULL },
        { { WTEXT("EXTRA SUBWAY STOP"), WTEXT("IN SILBERSTADT."), NULL }, NULL },
        { { WTEXT("AT ARGENT TOWERS?"), WTEXT("DORT IST NOCH EINER."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("JUST KEEP YOUR EYE"), WTEXT("ICH HOFFE, ICH HABE"), NULL }, NULL },
        { { WTEXT("ON THE MARKER."), WTEXT("DIR WEITERHELFEN"), NULL }, NULL },
        { { WTEXT("I HOPE THAT HELPS!"), WTEXT("KOENNEN!"), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("YOU'VE MANAGED TO KEEP"), WTEXT("DU HAST DEN WEG FUER"), NULL }, NULL },
        { { WTEXT("THE CARRIER SAFE? THAT'S"), WTEXT("DEN TRANSPORTER"), NULL }, NULL },
        { { WTEXT("PRETTY GOOD GOING!"), WTEXT("BEREITET? TOLLE SACHE!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("ALL WE HAVE TO DO NOW IS"), WTEXT("NUN MUESSEN WIR DIE"), NULL }, NULL },
        { { WTEXT("TRACK DOWN THE OTHER 0."), WTEXT("ANDEREN 0 NOCH FINDEN."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("I'M SURE I REMEMBER"), WTEXT("ICH GLAUBE, AN DER"), NULL }, NULL },
        { { WTEXT("HEARING ONE HAD MOVED"), WTEXT("ELFENBEINKUESTE LOHNT"), NULL }, NULL },
        { { WTEXT("TO THE EBONY COAST..."), WTEXT("SICH DIE SUCHE."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("YOU'LL HAVE TO STRIKE"), WTEXT("DU MUSST DICH IN DIE"), NULL }, NULL },
        { { WTEXT("OUT AHEAD AND GET "), WTEXT("LUFT BEGEBEN, UM IHN"), NULL }, NULL },
        { { WTEXT("AIRBORNE TO FIND HIM."), WTEXT("ZU FINDEN."), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("WHERE'S EVERYONE GONE?"), WTEXT("ICH BIN SO FROH, DASS"), NULL }, NULL },
        { { WTEXT("THERE WAS A LOT OF FUSS"), WTEXT("ICH WIEDER FRISCHE"), NULL }, NULL },
        { { WTEXT("OUT THERE A WHILE BACK."), WTEXT("LUFT ATMEN KANN."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("WHAT'S THAT? YOU'VE"), WTEXT("DU HAST 0 MEINER"), NULL }, NULL },
        { { WTEXT("GOT 0 OF MY FRIENDS"), WTEXT("FREUNDE IMMER NOCH"), NULL }, NULL },
        { { WTEXT("LEFT TO FIND?"), WTEXT("NICHT GEFUNDEN!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("I KNOW ONE OF THEM"), WTEXT("EINER SOLL SICH IN"), NULL }, NULL },
        { { WTEXT("LIVES AT TEMPEST CITY,"), WTEXT("TEMPCITY BEFINDEN!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("BUT THE NOISE IS TOO"), WTEXT("BEFREIE BITTE"), NULL }, NULL },
        { { WTEXT("MUCH FOR HIM."), WTEXT("AUCH IHN!!!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("HE LIKES TO GET ABOVE"), WTEXT("NUR WENN WIR ALLE"), NULL }, NULL },
        { { WTEXT("IT ALL AND SHUT"), WTEXT("ZUSAMMENARBEITEN,"), NULL }, NULL },
        { { WTEXT("HIMSELF AWAY."), WTEXT("RETTEN WIR DIE WELT."), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("HELP? OF COURSE I'LL"), WTEXT("HELFEN? NATUERLICH"), NULL }, NULL },
        { { WTEXT("HELP. FINALLY, SOMEONE'S"), WTEXT("WERDE ICH HELFEN! ICH"), NULL }, NULL },
        { { WTEXT("MAKING A STAND!"), WTEXT("TUE ES SOGAR GERN!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("THIS IS GOING TO TAKE"), WTEXT("DAZU BRAUCHEN WIR"), NULL }, NULL },
        { { WTEXT("ALL SIX OF US, SO"), WTEXT("ALLE SECHS. ES FEHLEN"), NULL }, NULL },
        { { WTEXT("YOU'LL NEED 0 MORE."), WTEXT("ALSO NOCH 0!!!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("TRY OYSTER HARBOUR."), WTEXT("IM AUSTERNHAFEN..."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("THIS UPROAR HAS LEFT"), WTEXT("... JA, GENAU DORT"), NULL }, NULL },
        { { WTEXT("MY COLLEAGUE THERE"), WTEXT("BEFINDET SICH MEIN"), NULL }, NULL },
        { { WTEXT("ALL OUT AT SEA,"), WTEXT("KOLLEGE."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("BUT YOU MUSTN'T LET"), WTEXT("MEINST DU, DU WIRST"), NULL }, NULL },
        { { WTEXT("ANYTHING STAND IN"), WTEXT("IHN NOCH RECHTZEITIG"), NULL }, NULL },
        { { WTEXT("YOUR WAY!"), WTEXT("RETTEN KOENNEN?"), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("WELL, IT'S GOOD TO SEE"), WTEXT("HEY, EIN NEUES GESICHT!"), NULL }, NULL },
        { { WTEXT("SOME NEW FACES! DON'T"), WTEXT("DICH KENNE ICH NOCH"), NULL }, NULL },
        { { WTEXT("MIND ME, LET'S MOVE OUT."), WTEXT("NICHT! INTERESSANT..."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("0 OF THE OTHER CHAPS"), WTEXT("0 DER ANDEREN SIND NOCH"), NULL }, NULL },
        { { WTEXT("LEFT TO FIND, AFTER ALL."), WTEXT("NICHT GEBORGEN WORDEN."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("I EXPECT THEY'VE DUCKED"), WTEXT("ICH GLAUBE, EIN BESUCH"), NULL }, NULL },
        { { WTEXT("FOR COVER UNDERGROUND"), WTEXT("DER EISENMINE WUERDE"), NULL }, NULL },
        { { WTEXT("AT IRONSTONE MINE."), WTEXT("SICH LOHNEN."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("YOU MIGHT NEED"), WTEXT("VIELLEICHT MUSST DU DIR"), NULL }, NULL },
        { { WTEXT("TO BLAST YOUR WAY DOWN."), WTEXT("DEN WEG FREISCHIESSEN!"), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { WTEXT("I CAN HARDLY BELIEVE"), WTEXT("UNGLAUBLICH, DIE ERDE"), NULL }, NULL },
        { { WTEXT("THE WORLD'S STILL IN"), WTEXT("IST NOCH AN EINEM"), NULL }, NULL },
        { { WTEXT("ONE PIECE!"), WTEXT("STUECK!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("STILL, AT LEAST NOW"), WTEXT("ENDLICH SIND WIR"), NULL }, NULL },
        { { WTEXT("WE'RE ALL BACK"), WTEXT("ALLE WIEDER"), NULL }, NULL },
        { { WTEXT("TOGETHER."), WTEXT("VEREINT."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("MAYBE WE FINALLY"), WTEXT("VIELLEICHT SIND WIR"), NULL }, NULL },
        { { WTEXT("STAND A CHANCE OF"), WTEXT("IN DER LAGE, DIE"), NULL }, NULL },
        { { WTEXT("CLEARING UP THIS MESS."), WTEXT("KATASTROPHE ABZUWENDEN."), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("NO TIME TO LOSE. LET'S"), WTEXT("KEINE ZEIT VERSCHWENDEN!"), NULL }, NULL },
        { { WTEXT("HEAD FOR THE DETONATION "), WTEXT("WIR HABEN NOCH VIEL ZU"), NULL }, NULL },
        { { WTEXT("SITE AND GET SET UP."), WTEXT("TUN, BEVOR ES LOSGEHT!"), NULL }, NULL },
        { { WTEXT(" "), WTEXT(" "), NULL }, NULL },
        { { WTEXT("WHEN IT COMES TO THE"), WTEXT("WENN ES ERNST WIRD,"), NULL }, NULL },
        { { WTEXT("CRUNCH, EVERYTHING'S"), WTEXT("LIEGT ALLES IN"), NULL }, NULL },
        { { WTEXT("GOING TO DEPEND ON US."), WTEXT("UNSEREN HAENDEN."), NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
#else
    {
        { WTEXT("WELL, IT'S ABOUT TIME!"), D_80303B9C },
        { WTEXT("DOES IT LOOK LIKE I'M"), D_80303BB4 },
        { WTEXT("ENJOYING MYSELF HERE?"), D_80303BC8 },
        { WTEXT(" "), D_80303BCC },
        { WTEXT("AND YOU'VE STILL GOT"), D_80303BD0 },
        { WTEXT("0 OF THE OTHERS"), D_80303BE4 },
        { WTEXT("LEFT TO TRACK DOWN."), D_80303BF8 },
        { WTEXT(" "), D_80303C14 },
        { WTEXT("VISIT GLORY CROSSING -"), D_80303C18 },
        { WTEXT("THERE'S BOUND TO BE"), D_80303C2C },
        { WTEXT("ONE HOLED UP THERE."), D_80303C48 },
        { WTEXT(" "), D_80303C4C },
        { WTEXT("IT SHOULDN'T TAKE YOU"), D_80303C50 },
        { WTEXT("TOO LONG TO FIND HIM."), D_80303C5C },
        { WTEXT(" "), D_80303C70 },
        { WTEXT(" "), D_80303C74 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { WTEXT("THIS IS IT, I TELL YOU!"), D_80303C78 },
        { WTEXT("THE END! WE'RE ALL"), D_80303C88 },
        { WTEXT("GOING TO ... OH."), D_80303C98 },
        { WTEXT(" "), D_80303CAC },
        { WTEXT("WHAT ABOUT THE REST"), D_80303CB0 },
        { WTEXT("OF MY FRIENDS?"), D_80303CC0 },
        { WTEXT("ONLY 0 MORE LEFT ..."), D_80303CD8 },
        { WTEXT(" "), D_80303CF0 },
        { WTEXT("WANT TO MAKE AN"), D_80303CF4 },
        { WTEXT("EXTRA SUBWAY STOP"), D_80303D0C },
        { WTEXT("AT ARGENT TOWERS?"), D_80303D28 },
        { WTEXT(" "), D_80303D40 },
        { WTEXT("JUST KEEP YOUR EYE"), D_80303D44 },
        { WTEXT("ON THE MARKER."), D_80303D58 },
        { WTEXT("I HOPE THAT HELPS!"), D_80303D70 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { WTEXT("YOU'VE MANAGED TO KEEP"), D_80303D80 },
        { WTEXT("THE CARRIER SAFE? THAT'S"), D_80303D98 },
        { WTEXT("PRETTY GOOD GOING!"), D_80303DA4 },
        { WTEXT(" "), D_80303DA8 },
        { WTEXT("ALL WE HAVE TO DO NOW IS"), D_80303DAC },
        { WTEXT("TRACK DOWN THE OTHER 0."), D_80303DC0 },
        { WTEXT(" "), D_80303DDC },
        { WTEXT("I'M SURE I REMEMBER"), D_80303DE0 },
        { WTEXT("HEARING ONE HAD MOVED"), D_80303DE4 },
        { WTEXT("TO THE EBONY COAST..."), D_80303DFC },
        { WTEXT(" "), D_80303E14 },
        { WTEXT("YOU'LL HAVE TO STRIKE"), D_80303E18 },
        { WTEXT("OUT AHEAD AND GET "), D_80303E30 },
        { WTEXT("AIRBORNE TO FIND HIM."), D_80303E48 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { WTEXT("WHERE'S EVERYONE GONE?"), D_80303E60 },
        { WTEXT("THERE WAS A LOT OF FUSS"), D_80303E70 },
        { WTEXT("OUT THERE A WHILE BACK."), D_80303E84 },
        { WTEXT(" "), D_80303E88 },
        { WTEXT("WHAT'S THAT? YOU'VE"), D_80303E8C },
        { WTEXT("GOT 0 OF MY FRIENDS"), D_80303EA0 },
        { WTEXT("LEFT TO FIND?"), D_80303EB8 },
        { WTEXT(" "), D_80303EBC },
        { WTEXT("I KNOW ONE OF THEM"), D_80303ED0 },
        { WTEXT("LIVES AT TEMPEST CITY,"), D_80303EE4 },
        { WTEXT(" "), D_80303EF4 },
        { WTEXT("BUT THE NOISE IS TOO"), D_80303EF8 },
        { WTEXT("MUCH FOR HIM."), D_80303F14 },
        { WTEXT(" "), D_80303F18 },
        { WTEXT("HE LIKES TO GET ABOVE"), D_80303F1C },
        { WTEXT("IT ALL AND SHUT"), D_80303F20 },
        { WTEXT("HIMSELF AWAY."), D_80303F24 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { WTEXT("HELP? OF COURSE I'LL"), D_80303F28 },
        { WTEXT("HELP. FINALLY, SOMEONE'S"), D_80303F34 },
        { WTEXT("MAKING A STAND!"), D_80303F4C },
        { WTEXT(" "), D_80303F50 },
        { WTEXT("THIS IS GOING TO TAKE"), D_80303F54 },
        { WTEXT("ALL SIX OF US, SO"), D_80303F6C },
        { WTEXT("YOU'LL NEED 0 MORE."), D_80303F84 },
        { WTEXT(" "), D_80303F88 },
        { WTEXT(" "), D_80303F8C },
        { WTEXT("TRY OYSTER HARBOR."), D_80303F9C },
        { WTEXT(" "), D_80303FB0 },
        { WTEXT(" "), D_80303FB4 },
        { WTEXT("THIS UPROAR HAS LEFT"), D_80303FB8 },
        { WTEXT("MY COLLEAGUE THERE"), D_80303FD4 },
        { WTEXT("ALL OUT AT SEA,"), D_80303FF0 },
        { WTEXT(" "), D_80303FF4 },
        { WTEXT("BUT YOU MUSTN'T LET"), D_80303FF8 },
        { WTEXT("ANYTHING STAND IN"), D_80303FFC },
        { WTEXT("YOUR WAY!"), D_80304000 },
        { NULL, NULL },
    },
    {
        { WTEXT("WELL, IT'S GOOD TO SEE"), D_80304004 },
        { WTEXT("SOME NEW FACES! DON'T"), D_8030401C },
        { WTEXT("MIND ME, LET'S MOVE OUT."), D_8030402C },
        { WTEXT(" "), D_80304030 },
        { WTEXT("0 OF THE OTHER CHAPS"), D_80304034 },
        { WTEXT("LEFT TO FIND, AFTER ALL."), D_8030404C },
        { WTEXT(" "), D_80304064 },
        { WTEXT(" "), D_80304080 },
        { WTEXT("I EXPECT THEY'VE DUCKED"), D_80304084 },
        { WTEXT("FOR COVER UNDERGROUND"), D_80304094 },
        { WTEXT("AT IRONSTONE MINE..."), D_803040A4 },
        { WTEXT(" "), D_803040A8 },
        { WTEXT("YOU MIGHT NEED"), D_803040AC },
        { WTEXT("TO BLAST YOUR WAY DOWN."), D_803040B0 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { WTEXT("I CAN HARDLY BELIEVE"), D_803040B4 },
        { WTEXT("THE WORLD'S STILL IN"), D_803040CC },
        { WTEXT("ONE PIECE!"), D_803040DC },
        { WTEXT(" "), D_803040E0 },
        { WTEXT("STILL, AT LEAST NOW"), D_803040E4 },
        { WTEXT("WE'RE ALL BACK"), D_803040F4 },
        { WTEXT("TOGETHER ..."), D_80304108 },
        { WTEXT(" "), D_80304118 },
        { WTEXT("MAYBE WE FINALLY"), D_8030411C },
        { WTEXT("STAND A CHANCE OF"), D_80304138 },
        { WTEXT("CLEARING UP THIS MESS."), D_80304154 },
        { WTEXT(" "), D_80304164 },
        { WTEXT("NO TIME TO LOSE. LET'S"), D_80304168 },
        { WTEXT("HEAD FOR THE DETONATION "), D_8030417C },
        { WTEXT("SITE AND GET SET UP."), D_80304190 },
        { WTEXT(" "), D_803041A8 },
        { WTEXT("WHEN IT COMES TO THE"), D_803041AC },
        { WTEXT("CRUNCH, EVERYTHING'S"), D_803041B0 },
        { WTEXT("GOING TO DEPEND ON US."), D_803041B4 },
        { NULL, NULL },
    },
#endif
};
/*
 * Window 21's selectable rows, per D_8039CAD0 (0 ends a short row): func_802979E0
 * flags the entries listed and starts the cursor on the first.  Two tables,
 * the second at 0x24 (D_802FF60C): us and eu use only the first, jp reads the
 * rows from the second and the cursor from the first.
 */
typedef struct {
    /* 0x00 */ u8 unk0[7][5];
    /* 0x23 */ u8 pad23;
    /* 0x24 */ u8 unk24[7][5];
} UnkStruct_802FF5E8; /* size = 0x47 */

UnkStruct_802FF5E8 D_802FF5E8 = {
    {
        { 0x01, 0x05, 0x09, 0x0D, 0x00 },
        { 0x01, 0x05, 0x09, 0x0E, 0x00 },
        { 0x01, 0x05, 0x08, 0x0C, 0x00 },
        { 0x01, 0x05, 0x09, 0x0C, 0x0F },
        { 0x01, 0x05, 0x09, 0x0D, 0x11 },
        { 0x01, 0x05, 0x09, 0x0D, 0x00 },
        { 0x01, 0x05, 0x09, 0x0D, 0x11 },
    },
    0x00,
    {
        { 0x01, 0x05, 0x09, 0x0D, 0x00 },
        { 0x01, 0x05, 0x09, 0x0D, 0x00 },
        { 0x01, 0x05, 0x09, 0x0C, 0x00 },
        { 0x01, 0x05, 0x08, 0x0B, 0x0E },
        { 0x01, 0x05, 0x09, 0x0D, 0x11 },
        { 0x01, 0x05, 0x09, 0x00, 0x00 },
        { 0x01, 0x05, 0x09, 0x0D, 0x00 },
    },
};

/* .bss, 0x8039CAD0-0x8039CAE0 (tools/bss_c.py) */
u8 D_8039CAD0;

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/53220/func_802979E0.s")
#else
void func_802979E0(u8 arg0) {
    YoshiWindow *sp4C = &D_802F8BDC[21];
    YoshiEntry *sp48;
    YoshiIcon *sp44;
    u8 sp43;
    u8 sp42;
    s32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;
    u8 sp2F;
    u8 *sp28;
    u16 *sp24;

    sp43 = func_80297F74();
    sp42 = 0;
    if (sp43 == 1 && func_80297EF8(arg0) == 0) {
        sp43--;
    }
    if (sp43 == 0) {
        D_8039CAD0 = 6;
    } else {
        sp2F = 0;
        for (sp3C = D_8039CAB6; sp3C < D_8039CAB6 + 6 && sp2F == 0;) {
            if (func_80297EF8(D_802FF180[sp3C % 6]) == 0 && D_802FF180[sp3C % 6] != arg0) {
                sp2F = 1;
            } else {
                sp3C++;
            }
        }
        D_8039CAD0 = sp3C % 6;
    }
    sp3C = 0;
    D_8036BB24 = (YoshiEntry *)D_80358070;
    D_80358070 += 21 * sizeof(YoshiEntry); /* 0x24C (eu 0x2F4) on the N64 */
    sp34 = 0;
    sp30 = 0;
    sp2F = 0;
    for (; sp3C < 20 && sp2F == 0; sp3C++) {
#ifdef VERSION_EU
        sp28 = D_802FF188[D_8039CAD0][sp3C].unk0[D_80366F70_eu];
#else
        sp28 = D_802FF188[D_8039CAD0][sp3C].unk0;
#endif
        sp24 = D_802FF188[D_8039CAD0][sp3C].unk4;
        sp48 = &D_8036BB24[sp3C];
        if (sp28 != NULL) {
            sp30++;
            sp48->flags = 0x1020;
#ifdef TARGET_PC
            /* a full row (5 entries) has no 0 after it: once all 5 are
               found, look no further (the N64 reads the next row's first
               byte, or the pad after the last row, which never match) */
            if (sp34 < sizeof(D_802FF5E8.unk0[0]) && D_802FF5E8.unk0[D_8039CAD0][sp34] == sp3C) {
#else
            if (D_802FF5E8.unk0[D_8039CAD0][sp34] == sp3C) {
#endif
                sp48->flags |= 1;
                sp34++;
            }
            sp48->y = sp3C * 16;
            sp48->unk6 = 16;
            sp48->unk8 = 16;
            ENTRY_TEXT(sp48) = (char *)sp28;
            sp48->unk10 = sp24;
            sp48->unk14 = 0;
            sp48->unk16 = 0;
            sp48->unk18 = 7;
            sp48->unk19 = 7;
            sp48->unk1A = 0;
        } else {
            sp2F = 1;
        }
    }
    sp48 = &D_8036BB24[sp30];
    ENTRY_TEXT(sp48) = NULL;
    sp48->unk10 = NULL;
    sp48->flags = 0x400;
    sp48->x = -0x20;
    sp48->y = 0x26;
    sp48->unk14 = 0x18;
#ifdef TARGET_PC
    /* its colours, which the N64 leaves as the heap held them (26570.c) */
    sp48->unk18 = sp48->unk19 = 7;
#endif
    sp48->unk1A = 0;
    sp48->unk16 = (u8)sp48->unk1A;
    sp44 = &D_802F49F4[sp48->unk14];
    sp48->unk1A = func_80272C5C((u16 *)sp44->unk6, 0, sp44->unk4, sp44->unk2C, sp44->unk2D | 4, 1.0f);
    sp4C->count = sp30 + 1;
    sp4C->unk18 = D_802FF5E8.unk0[D_8039CAD0][0];
    if (func_80297EF8(arg0) == 0) {
        sp43--;
    }
    for (sp3C = 0; sp3C < sp4C->count && sp42 == 0; sp3C++) {
        sp48 = &D_8036BB24[sp3C];
        for (sp38 = 0; sp38 < func_8025B300((u8 *)ENTRY_TEXT(sp48)) && sp42 == 0; sp38++) {
            if (((u8 *)ENTRY_TEXT(sp48))[sp38] >= '0' && ((u8 *)ENTRY_TEXT(sp48))[sp38] < '6') {
                ENTRY_TEXT(sp48)[sp38] = sp43 + '0';
                sp42 = 1;
            }
        }
    }
}
#endif

void func_80297ECC(void) {
    D_802F8BDC[21].unk18 = D_802FF5E8.unk0[D_8039CAD0][0];
}

u8 func_80297EF8(u8 arg0) {
    u8 sp27;
    u8 sp26;

    sp26 = func_8029766C(arg0, &sp27);
    return (sp26 != 0 && (D_80364AF0[D_80364AE8].unk90 & (1 << sp27))) ? 1 : 0;
}

u8 func_80297F74(void) {
    s32 sp4;
    s32 sp0 = 6;

    for (sp4 = 0; sp4 < 6; sp4++) {
        if (D_80364AF0[D_80364AE8].unk90 & (1 << sp4)) {
            sp0--;
        }
    }
    return sp0;
}
