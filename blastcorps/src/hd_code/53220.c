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
UnkStruct_802FF188 D_802FF188[7][20] = {
#ifdef VERSION_EU
    {
        { { (u8 *)"WELL, IT'S ABOUT TIME!", (u8 *)"HEY, WIRD ABER AUCH", NULL }, NULL },
        { { (u8 *)"DOES IT LOOK LIKE I'M", (u8 *)"ZEIT, DASS MICH JEMAND", NULL }, NULL },
        { { (u8 *)"ENJOYING MYSELF HERE?", (u8 *)"HIER HERAUSHOLT!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"AND YOU'VE STILL GOT", (u8 *)"NOCH SIND GENAU", NULL }, NULL },
        { { (u8 *)"0 OF THE OTHERS", (u8 *)"0 MEINER KOLLEGEN", NULL }, NULL },
        { { (u8 *)"LEFT TO TRACK DOWN.", (u8 *)"NICHT GERETTET!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"VISIT GLORY CROSSING -", (u8 *)"SCHAU IN DER KATENA-", NULL }, NULL },
        { { (u8 *)"THERE'S BOUND TO BE", (u8 *)"SCHLUCHT NACH. DORT", NULL }, NULL },
        { { (u8 *)"ONE HOLED UP THERE.", (u8 *)"IST EIN GEFANGENER.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"IT SHOULDN'T TAKE YOU", (u8 *)"BEEILE DICH, IHN", NULL }, NULL },
        { { (u8 *)"TOO LONG TO FIND HIM.", (u8 *)"ZU FINDEN!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"THIS IS IT, I TELL YOU!", (u8 *)"DAS IST DAS ENDE, WIR", NULL }, NULL },
        { { (u8 *)"THE END! WE'RE ALL", (u8 *)"WERDEN ALLE...", NULL }, NULL },
        { { (u8 *)"GOING TO... OH.", (u8 *)"OH MEIN...", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"WHAT ABOUT THE REST", (u8 *)"WAS IST MIT DEN", NULL }, NULL },
        { { (u8 *)"OF MY FRIENDS?", (u8 *)"ANDEREN? DU MUSST", NULL }, NULL },
        { { (u8 *)"ONLY 0 MORE LEFT!", (u8 *)"SIE RETTEN!!!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"WANT TO MAKE AN", (u8 *)"MACHE EINEN STOP", NULL }, NULL },
        { { (u8 *)"EXTRA SUBWAY STOP", (u8 *)"IN SILBERSTADT.", NULL }, NULL },
        { { (u8 *)"AT ARGENT TOWERS?", (u8 *)"DORT IST NOCH EINER.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"JUST KEEP YOUR EYE", (u8 *)"ICH HOFFE, ICH HABE", NULL }, NULL },
        { { (u8 *)"ON THE MARKER.", (u8 *)"DIR WEITERHELFEN", NULL }, NULL },
        { { (u8 *)"I HOPE THAT HELPS!", (u8 *)"KOENNEN!", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"YOU'VE MANAGED TO KEEP", (u8 *)"DU HAST DEN WEG FUER", NULL }, NULL },
        { { (u8 *)"THE CARRIER SAFE? THAT'S", (u8 *)"DEN TRANSPORTER", NULL }, NULL },
        { { (u8 *)"PRETTY GOOD GOING!", (u8 *)"BEREITET? TOLLE SACHE!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"ALL WE HAVE TO DO NOW IS", (u8 *)"NUN MUESSEN WIR DIE", NULL }, NULL },
        { { (u8 *)"TRACK DOWN THE OTHER 0.", (u8 *)"ANDEREN 0 NOCH FINDEN.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"I'M SURE I REMEMBER", (u8 *)"ICH GLAUBE, AN DER", NULL }, NULL },
        { { (u8 *)"HEARING ONE HAD MOVED", (u8 *)"ELFENBEINKUESTE LOHNT", NULL }, NULL },
        { { (u8 *)"TO THE EBONY COAST...", (u8 *)"SICH DIE SUCHE.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"YOU'LL HAVE TO STRIKE", (u8 *)"DU MUSST DICH IN DIE", NULL }, NULL },
        { { (u8 *)"OUT AHEAD AND GET ", (u8 *)"LUFT BEGEBEN, UM IHN", NULL }, NULL },
        { { (u8 *)"AIRBORNE TO FIND HIM.", (u8 *)"ZU FINDEN.", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"WHERE'S EVERYONE GONE?", (u8 *)"ICH BIN SO FROH, DASS", NULL }, NULL },
        { { (u8 *)"THERE WAS A LOT OF FUSS", (u8 *)"ICH WIEDER FRISCHE", NULL }, NULL },
        { { (u8 *)"OUT THERE A WHILE BACK.", (u8 *)"LUFT ATMEN KANN.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"WHAT'S THAT? YOU'VE", (u8 *)"DU HAST 0 MEINER", NULL }, NULL },
        { { (u8 *)"GOT 0 OF MY FRIENDS", (u8 *)"FREUNDE IMMER NOCH", NULL }, NULL },
        { { (u8 *)"LEFT TO FIND?", (u8 *)"NICHT GEFUNDEN!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"I KNOW ONE OF THEM", (u8 *)"EINER SOLL SICH IN", NULL }, NULL },
        { { (u8 *)"LIVES AT TEMPEST CITY,", (u8 *)"TEMPCITY BEFINDEN!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"BUT THE NOISE IS TOO", (u8 *)"BEFREIE BITTE", NULL }, NULL },
        { { (u8 *)"MUCH FOR HIM.", (u8 *)"AUCH IHN!!!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"HE LIKES TO GET ABOVE", (u8 *)"NUR WENN WIR ALLE", NULL }, NULL },
        { { (u8 *)"IT ALL AND SHUT", (u8 *)"ZUSAMMENARBEITEN,", NULL }, NULL },
        { { (u8 *)"HIMSELF AWAY.", (u8 *)"RETTEN WIR DIE WELT.", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"HELP? OF COURSE I'LL", (u8 *)"HELFEN? NATUERLICH", NULL }, NULL },
        { { (u8 *)"HELP. FINALLY, SOMEONE'S", (u8 *)"WERDE ICH HELFEN! ICH", NULL }, NULL },
        { { (u8 *)"MAKING A STAND!", (u8 *)"TUE ES SOGAR GERN!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"THIS IS GOING TO TAKE", (u8 *)"DAZU BRAUCHEN WIR", NULL }, NULL },
        { { (u8 *)"ALL SIX OF US, SO", (u8 *)"ALLE SECHS. ES FEHLEN", NULL }, NULL },
        { { (u8 *)"YOU'LL NEED 0 MORE.", (u8 *)"ALSO NOCH 0!!!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"TRY OYSTER HARBOUR.", (u8 *)"IM AUSTERNHAFEN...", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"THIS UPROAR HAS LEFT", (u8 *)"... JA, GENAU DORT", NULL }, NULL },
        { { (u8 *)"MY COLLEAGUE THERE", (u8 *)"BEFINDET SICH MEIN", NULL }, NULL },
        { { (u8 *)"ALL OUT AT SEA,", (u8 *)"KOLLEGE.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"BUT YOU MUSTN'T LET", (u8 *)"MEINST DU, DU WIRST", NULL }, NULL },
        { { (u8 *)"ANYTHING STAND IN", (u8 *)"IHN NOCH RECHTZEITIG", NULL }, NULL },
        { { (u8 *)"YOUR WAY!", (u8 *)"RETTEN KOENNEN?", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"WELL, IT'S GOOD TO SEE", (u8 *)"HEY, EIN NEUES GESICHT!", NULL }, NULL },
        { { (u8 *)"SOME NEW FACES! DON'T", (u8 *)"DICH KENNE ICH NOCH", NULL }, NULL },
        { { (u8 *)"MIND ME, LET'S MOVE OUT.", (u8 *)"NICHT! INTERESSANT...", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"0 OF THE OTHER CHAPS", (u8 *)"0 DER ANDEREN SIND NOCH", NULL }, NULL },
        { { (u8 *)"LEFT TO FIND, AFTER ALL.", (u8 *)"NICHT GEBORGEN WORDEN.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"I EXPECT THEY'VE DUCKED", (u8 *)"ICH GLAUBE, EIN BESUCH", NULL }, NULL },
        { { (u8 *)"FOR COVER UNDERGROUND", (u8 *)"DER EISENMINE WUERDE", NULL }, NULL },
        { { (u8 *)"AT IRONSTONE MINE.", (u8 *)"SICH LOHNEN.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"YOU MIGHT NEED", (u8 *)"VIELLEICHT MUSST DU DIR", NULL }, NULL },
        { { (u8 *)"TO BLAST YOUR WAY DOWN.", (u8 *)"DEN WEG FREISCHIESSEN!", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
    {
        { { (u8 *)"I CAN HARDLY BELIEVE", (u8 *)"UNGLAUBLICH, DIE ERDE", NULL }, NULL },
        { { (u8 *)"THE WORLD'S STILL IN", (u8 *)"IST NOCH AN EINEM", NULL }, NULL },
        { { (u8 *)"ONE PIECE!", (u8 *)"STUECK!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"STILL, AT LEAST NOW", (u8 *)"ENDLICH SIND WIR", NULL }, NULL },
        { { (u8 *)"WE'RE ALL BACK", (u8 *)"ALLE WIEDER", NULL }, NULL },
        { { (u8 *)"TOGETHER.", (u8 *)"VEREINT.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"MAYBE WE FINALLY", (u8 *)"VIELLEICHT SIND WIR", NULL }, NULL },
        { { (u8 *)"STAND A CHANCE OF", (u8 *)"IN DER LAGE, DIE", NULL }, NULL },
        { { (u8 *)"CLEARING UP THIS MESS.", (u8 *)"KATASTROPHE ABZUWENDEN.", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"NO TIME TO LOSE. LET'S", (u8 *)"KEINE ZEIT VERSCHWENDEN!", NULL }, NULL },
        { { (u8 *)"HEAD FOR THE DETONATION ", (u8 *)"WIR HABEN NOCH VIEL ZU", NULL }, NULL },
        { { (u8 *)"SITE AND GET SET UP.", (u8 *)"TUN, BEVOR ES LOSGEHT!", NULL }, NULL },
        { { (u8 *)" ", (u8 *)" ", NULL }, NULL },
        { { (u8 *)"WHEN IT COMES TO THE", (u8 *)"WENN ES ERNST WIRD,", NULL }, NULL },
        { { (u8 *)"CRUNCH, EVERYTHING'S", (u8 *)"LIEGT ALLES IN", NULL }, NULL },
        { { (u8 *)"GOING TO DEPEND ON US.", (u8 *)"UNSEREN HAENDEN.", NULL }, NULL },
        { { NULL, NULL, NULL }, NULL },
    },
#else
    {
        { (u8 *)"WELL, IT'S ABOUT TIME!", D_80303B9C },
        { (u8 *)"DOES IT LOOK LIKE I'M", D_80303BB4 },
        { (u8 *)"ENJOYING MYSELF HERE?", D_80303BC8 },
        { (u8 *)" ", D_80303BCC },
        { (u8 *)"AND YOU'VE STILL GOT", D_80303BD0 },
        { (u8 *)"0 OF THE OTHERS", D_80303BE4 },
        { (u8 *)"LEFT TO TRACK DOWN.", D_80303BF8 },
        { (u8 *)" ", D_80303C14 },
        { (u8 *)"VISIT GLORY CROSSING -", D_80303C18 },
        { (u8 *)"THERE'S BOUND TO BE", D_80303C2C },
        { (u8 *)"ONE HOLED UP THERE.", D_80303C48 },
        { (u8 *)" ", D_80303C4C },
        { (u8 *)"IT SHOULDN'T TAKE YOU", D_80303C50 },
        { (u8 *)"TOO LONG TO FIND HIM.", D_80303C5C },
        { (u8 *)" ", D_80303C70 },
        { (u8 *)" ", D_80303C74 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { (u8 *)"THIS IS IT, I TELL YOU!", D_80303C78 },
        { (u8 *)"THE END! WE'RE ALL", D_80303C88 },
        { (u8 *)"GOING TO ... OH.", D_80303C98 },
        { (u8 *)" ", D_80303CAC },
        { (u8 *)"WHAT ABOUT THE REST", D_80303CB0 },
        { (u8 *)"OF MY FRIENDS?", D_80303CC0 },
        { (u8 *)"ONLY 0 MORE LEFT ...", D_80303CD8 },
        { (u8 *)" ", D_80303CF0 },
        { (u8 *)"WANT TO MAKE AN", D_80303CF4 },
        { (u8 *)"EXTRA SUBWAY STOP", D_80303D0C },
        { (u8 *)"AT ARGENT TOWERS?", D_80303D28 },
        { (u8 *)" ", D_80303D40 },
        { (u8 *)"JUST KEEP YOUR EYE", D_80303D44 },
        { (u8 *)"ON THE MARKER.", D_80303D58 },
        { (u8 *)"I HOPE THAT HELPS!", D_80303D70 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { (u8 *)"YOU'VE MANAGED TO KEEP", D_80303D80 },
        { (u8 *)"THE CARRIER SAFE? THAT'S", D_80303D98 },
        { (u8 *)"PRETTY GOOD GOING!", D_80303DA4 },
        { (u8 *)" ", D_80303DA8 },
        { (u8 *)"ALL WE HAVE TO DO NOW IS", D_80303DAC },
        { (u8 *)"TRACK DOWN THE OTHER 0.", D_80303DC0 },
        { (u8 *)" ", D_80303DDC },
        { (u8 *)"I'M SURE I REMEMBER", D_80303DE0 },
        { (u8 *)"HEARING ONE HAD MOVED", D_80303DE4 },
        { (u8 *)"TO THE EBONY COAST...", D_80303DFC },
        { (u8 *)" ", D_80303E14 },
        { (u8 *)"YOU'LL HAVE TO STRIKE", D_80303E18 },
        { (u8 *)"OUT AHEAD AND GET ", D_80303E30 },
        { (u8 *)"AIRBORNE TO FIND HIM.", D_80303E48 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { (u8 *)"WHERE'S EVERYONE GONE?", D_80303E60 },
        { (u8 *)"THERE WAS A LOT OF FUSS", D_80303E70 },
        { (u8 *)"OUT THERE A WHILE BACK.", D_80303E84 },
        { (u8 *)" ", D_80303E88 },
        { (u8 *)"WHAT'S THAT? YOU'VE", D_80303E8C },
        { (u8 *)"GOT 0 OF MY FRIENDS", D_80303EA0 },
        { (u8 *)"LEFT TO FIND?", D_80303EB8 },
        { (u8 *)" ", D_80303EBC },
        { (u8 *)"I KNOW ONE OF THEM", D_80303ED0 },
        { (u8 *)"LIVES AT TEMPEST CITY,", D_80303EE4 },
        { (u8 *)" ", D_80303EF4 },
        { (u8 *)"BUT THE NOISE IS TOO", D_80303EF8 },
        { (u8 *)"MUCH FOR HIM.", D_80303F14 },
        { (u8 *)" ", D_80303F18 },
        { (u8 *)"HE LIKES TO GET ABOVE", D_80303F1C },
        { (u8 *)"IT ALL AND SHUT", D_80303F20 },
        { (u8 *)"HIMSELF AWAY.", D_80303F24 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { (u8 *)"HELP? OF COURSE I'LL", D_80303F28 },
        { (u8 *)"HELP. FINALLY, SOMEONE'S", D_80303F34 },
        { (u8 *)"MAKING A STAND!", D_80303F4C },
        { (u8 *)" ", D_80303F50 },
        { (u8 *)"THIS IS GOING TO TAKE", D_80303F54 },
        { (u8 *)"ALL SIX OF US, SO", D_80303F6C },
        { (u8 *)"YOU'LL NEED 0 MORE.", D_80303F84 },
        { (u8 *)" ", D_80303F88 },
        { (u8 *)" ", D_80303F8C },
        { (u8 *)"TRY OYSTER HARBOR.", D_80303F9C },
        { (u8 *)" ", D_80303FB0 },
        { (u8 *)" ", D_80303FB4 },
        { (u8 *)"THIS UPROAR HAS LEFT", D_80303FB8 },
        { (u8 *)"MY COLLEAGUE THERE", D_80303FD4 },
        { (u8 *)"ALL OUT AT SEA,", D_80303FF0 },
        { (u8 *)" ", D_80303FF4 },
        { (u8 *)"BUT YOU MUSTN'T LET", D_80303FF8 },
        { (u8 *)"ANYTHING STAND IN", D_80303FFC },
        { (u8 *)"YOUR WAY!", D_80304000 },
        { NULL, NULL },
    },
    {
        { (u8 *)"WELL, IT'S GOOD TO SEE", D_80304004 },
        { (u8 *)"SOME NEW FACES! DON'T", D_8030401C },
        { (u8 *)"MIND ME, LET'S MOVE OUT.", D_8030402C },
        { (u8 *)" ", D_80304030 },
        { (u8 *)"0 OF THE OTHER CHAPS", D_80304034 },
        { (u8 *)"LEFT TO FIND, AFTER ALL.", D_8030404C },
        { (u8 *)" ", D_80304064 },
        { (u8 *)" ", D_80304080 },
        { (u8 *)"I EXPECT THEY'VE DUCKED", D_80304084 },
        { (u8 *)"FOR COVER UNDERGROUND", D_80304094 },
        { (u8 *)"AT IRONSTONE MINE...", D_803040A4 },
        { (u8 *)" ", D_803040A8 },
        { (u8 *)"YOU MIGHT NEED", D_803040AC },
        { (u8 *)"TO BLAST YOUR WAY DOWN.", D_803040B0 },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },
    {
        { (u8 *)"I CAN HARDLY BELIEVE", D_803040B4 },
        { (u8 *)"THE WORLD'S STILL IN", D_803040CC },
        { (u8 *)"ONE PIECE!", D_803040DC },
        { (u8 *)" ", D_803040E0 },
        { (u8 *)"STILL, AT LEAST NOW", D_803040E4 },
        { (u8 *)"WE'RE ALL BACK", D_803040F4 },
        { (u8 *)"TOGETHER ...", D_80304108 },
        { (u8 *)" ", D_80304118 },
        { (u8 *)"MAYBE WE FINALLY", D_8030411C },
        { (u8 *)"STAND A CHANCE OF", D_80304138 },
        { (u8 *)"CLEARING UP THIS MESS.", D_80304154 },
        { (u8 *)" ", D_80304164 },
        { (u8 *)"NO TIME TO LOSE. LET'S", D_80304168 },
        { (u8 *)"HEAD FOR THE DETONATION ", D_8030417C },
        { (u8 *)"SITE AND GET SET UP.", D_80304190 },
        { (u8 *)" ", D_803041A8 },
        { (u8 *)"WHEN IT COMES TO THE", D_803041AC },
        { (u8 *)"CRUNCH, EVERYTHING'S", D_803041B0 },
        { (u8 *)"GOING TO DEPEND ON US.", D_803041B4 },
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
            if (D_802FF5E8.unk0[D_8039CAD0][sp34] == sp3C) {
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
