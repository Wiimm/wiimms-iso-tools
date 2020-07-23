
#define _GNU_SOURCE 1

#include "dclib/dclib-debug.h"
#include "wiidisc.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "iso-interface.h"

#include "ui-wit.h"

//-----------------------------------------------------------------------------

void print_title ( FILE * f );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			debugging			///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef TEST
    #define DEBUG_SM	0 // debug sector mix
#else
    #define DEBUG_SM	0 // debug sector mix
#endif

#if DEBUG_SM
    #define PRINT_SM PRINT
#else
    #define PRINT_SM noPRINT
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Mix_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct Mix_t
{
    //--- input data

    ccp		source;		// pointer to source file ( := param->arg )
    u32		ptab;		// destination partition tables
    u32		ptype;		// destination partition type
    wd_trim_mode_t trim;	// trim mode
    bool	is_boot_part;	// true: special boot partition (always at index #0)

    //--- secondary data
    
    SuperFile_t	* sf;		// superfile of iso image
    bool	have_pat;	// true if partitions have ignore pattern
    bool	free_data;	// true: free 'sf'
    wd_disc_t	* disc;		// valid disc pointer
    wd_part_t	* part;		// valid partition pointer

    u32		src_sector;	// index of first source sector
    u32		src_end_sector;	// index of last source sector + 1
    u32		dest_sector;	// index of first destination sector
    u32		n_blocks;	// number of continuous sector blocks

    //--- Permutation helpers

    u32		a1size;		// size of area 1 (p1off allways = 0)
    u32		a2off;		// offset of area 2
    u32		a2size;		// size of area 2

    //--- sector mix data

    u32		* used_free;	// 2*n_blocks+1 used and free blocks counts
    
} Mix_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MixFree_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct MixFree_t
{
	u32 off;		// offset of first free sector
	u32 size;		// number of free sectors

} MixFree_t;


//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MixParam_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define MAX_MIX_PERM 12  // = 479 001 600 permutations

enum { NO_OVERLAY, SIMPLE_OVERLAY, MULTI_HOLE_OVERLAY };

static ccp overlay_info[] =
{
	"no overlay",
	"simple overlay",
	"multiple hole overlay",
	0
};

//-----------------------------------------------------------------------------

typedef struct MixParam_t
{
    //----- base parameters

    Mix_t	mix[WII_MAX_PARTITIONS];	// valid pointer to table
    int		n_mix;				// number of valid elements of 'mix'
    Mix_t	* end_mix;			// := mix + n_mix

    wd_disc_type_t disc_type;			// disc type of new image
    bool	is_gc;				// true: is gamecube mix
    bool	is_multiboot;			// true: is gamecube multiboot mix
    bool	have_boot_part;			// true: have a gamecube boot partition
    int		overlay;			// overlay mode

    u32		start_sector;			// start sector of first partition
    u32		sector_align;			// align partition start
    u32		max_sector;			// last used sector + 1

    wd_header_t	* source_dhead;			// dhead source
    wd_region_t	* source_region;		// region source


    //----- output file

    SuperFile_t	fo;				// output file
    u64		dest_file_size;			// projected destination file size
    MemMap_t	mm;				// memory mapping


    //----- parameters for overlay calculations    

    int		used[MAX_MIX_PERM];
    MixFree_t	field[MAX_MIX_PERM][2*MAX_MIX_PERM+2];

    u32		delta[MAX_MIX_PERM];
    u32		max_end;

} MixParam_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			verify_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError verify_mix
(
    MixParam_t	* mp,		// valid pointer to mix param + data
    bool	dump,		// true: dump usage table
    bool	create_memmap	// true: create a memory map
)
{
    ASSERT(mp);
    
    u8 utab[WII_MAX_SECTORS];
    memset(utab,WD_USAGE_UNUSED,sizeof(utab));
    if (mp->is_gc)
    {
	// [[2do]] [gc]
    }
    else
    {
	utab[ 0 ]					= WD_USAGE_DISC;
	utab[ WII_PTAB_REF_OFF / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;
	utab[ WII_REGION_OFF   / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;
	utab[ WII_MAGIC2_OFF   / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;
    }

    const int part_fw = sprintf(iobuf,"%d",mp->n_mix-1);

    u32 error_count = 0;
    u8 dest_id = WD_USAGE_PART_0;
    Mix_t *mix;
    for ( mix = mp->mix; mix < mp->end_mix; mix++, dest_id++ )
    {
	wd_part_t * part = mix->part;
	DASSERT(part);
	const u8 src_id = part->usage_id;

	const int max_sectors = WII_MAX_SECTORS - ( mix->src_sector > mix->dest_sector
						? mix->src_sector : mix->dest_sector );
	u8 *dest = utab + mix->dest_sector, *enddest = dest + max_sectors;
	const u8 *src = mix->disc->usage_table + mix->src_sector;

	noPRINT(">> V#%zu: sect=%5x..%5x -> %5x..%5x, n=%5x\n",
		mix - mp->mix,
		mix->src_sector, mix->src_sector + max_sectors,
		mix->dest_sector, mix->dest_sector + max_sectors,
		max_sectors );

    #ifdef xxxDEBUG
	wd_print_usage_tab(stdout,2,utab,0,false);
	PRINT("--\n");
	wd_print_disc_usage_tab(stdout,2,mix->disc,false);
    #endif

	for ( ; dest < enddest; dest++, src++ )
	{
	    if ( ( *src & WD_USAGE__MASK ) == src_id )
	    {
		u8 * start = dest;
		while ( dest < enddest && ( *src & WD_USAGE__MASK ) == src_id )
		{
		    if (*dest)
		    {
			PRINT_IF( error_count < 20,
				"* MIX VERIFY ERROR: mix #%zu, dest[%zx]=%x, planned=%x\n",
				mix - mp->mix, dest - utab,
				*dest, dest_id | *src++ & WD_USAGE_F_CRYPT );
			error_count++;
		    }
		    *dest++ = dest_id | *src++ & WD_USAGE_F_CRYPT;
		}

		if (create_memmap)
		{
		    if ( mp->is_gc && start == utab )
			start++; // first block of gc discs is handled separately

		    if ( start < dest )
		    {
			MemMapItem_t * item
			    = InsertMemMap( &mp->mm,
					    ( start - utab ) * (u64)WII_SECTOR_SIZE,
					    ( dest - start ) * (u64)WII_SECTOR_SIZE );
			DASSERT(item);
			item->index = mix - mp->mix;
			if (!mp->is_gc)
			    snprintf(item->info,sizeof(item->info),
				"P.%0*u, %s, %s -> %s",
				part_fw, (int)(mix-mp->mix),
				wd_print_id(&mix->disc->dhead.disc_id,6,0),
				wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				wd_print_part_name(0,0,mix->ptype,WD_PNAME_NUM_INFO) );
			else if (mix->is_boot_part)
			    snprintf(item->info,sizeof(item->info),
				"Boot, %s",
				wd_print_id(&mix->disc->dhead.disc_id,6,0) );
			else
			    snprintf(item->info,sizeof(item->info),
				"P.%0*u, %s",
				part_fw, (int)(mix-mp->mix),
				wd_print_id(&mix->disc->dhead.disc_id,6,0) );
		    }
		}
	    }
	}
    }

    if ( mp->is_gc && !*utab )
	*utab = WD_USAGE_DISC;

    if (dump)
    {
	printf("Usage table:\n\n");
	wd_print_usage_tab(stdout,2,utab,0,false);
	putchar('\n');
    }

    if (error_count)
	return ERROR0(ERR_INTERNAL,
		"Verification of mix failed for %u sector%s!\n",
		error_count, error_count == 1 ? "" : "s" );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			mix_scan_param()		///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError scan_mix_param
(
    MixParam_t	* mp		// valid pointer to mix param + data
)
{
    DASSERT(mp);
    mp->disc_type = WD_DT_UNKNOWN;

    u32 max_part = InfoUI_wit.opt_used[OPT_OVERLAY]
			? MAX_MIX_PERM
			: WII_MAX_PARTITIONS;

    u32 ptab_count[WII_MAX_PTAB];

    bool src_warn = true;
    ParamList_t * param = first_param;
    while (param)
    {
	//--- scan source file

	AtExpandParam(&param);
	ccp srcfile = param->arg;
	if ( src_warn && !strchr(srcfile,'/') && !strchr(srcfile,'.') )
	{
	    src_warn = false;
	    ERROR0(ERR_WARNING,
		"Warning: use at least one '.' or '/' in a filename"
		" to distinguish file names from keywords: %s", srcfile );
	}
	param = param->next;


	//--- scan qualifiers
	
	bool scan_qualifier = true;

	wd_select_t psel;
	wd_initialize_select(&psel);
	bool qual_select = false;

	u32 ptab = 0, ptype = 0;
	bool ptab_valid = false, ptype_valid = false;
	bool pattern_active = false;
	ResetFilePattern( file_pattern + PAT_PARAM );

	bool qual_boot = false, qual_header = false, qual_region = false;
	wd_trim_mode_t trim = opt_trim; // [doc]

	while ( scan_qualifier )
	{
	    scan_qualifier = false;
	    AtExpandParam(&param);

	    //--- scan 'SELECT'

	    if ( param && param->next
		 && (  !strcasecmp(param->arg,"select")
		    || !strcasecmp(param->arg,"psel") ))
	    {
		param = param->next;
		AtExpandParam(&param);
		const enumError err = ScanPartSelector(&psel,param->arg," ('select')");
		if (err)
		    return err;
		param = param->next;
		scan_qualifier = qual_select = true;
	    }


	    //--- scan 'TRIM' [doc]

	    if ( param && !strcasecmp(param->arg,"trim") )
	    {
		param = param->next;
		AtExpandParam(&param);
		trim = ScanTrim(param->arg," (qualifier 'trim')");
		if ( trim == (wd_trim_mode_t)-1 )
		    return ERR_SYNTAX;
		param = param->next;
		scan_qualifier = true;
	    }


	    //--- scan 'AS'

	    if ( param && param->next && !strcasecmp(param->arg,"as") )
	    {
		param = param->next;
		AtExpandParam(&param);
		const enumError err
		    = ScanPartTabAndType(&ptab,&ptab_valid,&ptype,&ptype_valid,
					    param->arg," ('as')");
		if (err)
		    return err;
		param = param->next;
		scan_qualifier = true;
	    }


	    //--- scan 'IGNORE'

	    if ( param && param->next && !strcasecmp(param->arg,"ignore") )
	    {
		param = param->next;
		if (AtFileHelper(param->arg,PAT_PARAM,PAT_PARAM,AddFilePattern))
		    return ERR_SYNTAX;
		param = param->next;
		pattern_active = true;
		scan_qualifier = true;
	    }


	    //--- scan 'BOOT' // [doc]

	    if ( param && !strcasecmp(param->arg,"boot") )
	    {
		param = param->next;
		qual_boot = true;
		scan_qualifier = true;

		if (mp->have_boot_part)
		    return ERROR0(ERR_SEMANTIC,
			"Multiple usage of qualifier 'boot' is not allowed.");
	    }


	    //--- scan 'HEADER'

	    if ( param && !strcasecmp(param->arg,"header") )
	    {
		param = param->next;
		qual_header = true;
		scan_qualifier = true;

		if (mp->source_dhead)
		    return ERROR0(ERR_SEMANTIC,
			"Multiple usage of qualifier 'header' is not allowed.");
	    }


	    //--- scan 'REGION'

	    if ( param && !strcasecmp(param->arg,"region") )
	    {
		param = param->next;
		qual_region = true;
		scan_qualifier = true;

		if (mp->source_region)
		    return ERROR0(ERR_SEMANTIC,
			"Multiple usage of qualifier 'region' is not allowed.");
	    }
	}


	//--- analyze pattern

	FilePattern_t * pat = 0;
	if (pattern_active)
	{
	    pat = file_pattern + PAT_PARAM;
	    SetupFilePattern(pat);
	    DefineNegatePattern(pat,true);
	    if (!pat->rules.used)
		pat = 0;
	}


	//--- open disc and partitions

	SuperFile_t * sf = AllocSF();
	ASSERT(sf);
	enumError err = OpenSF(sf,srcfile,false,false);
	if (err)
	    return err;

	wd_disc_t * disc = OpenDiscSF(sf,false,true);
	if (!disc)
	    return ERR_WDISC_NOT_FOUND;

	if ( mp->disc_type == WD_DT_UNKNOWN )
	{
	    mp->disc_type = disc->disc_type;
	    mp->is_gc = mp->disc_type == WD_DT_GAMECUBE;
	    if ( mp->is_gc && max_part > GC_MULTIBOOT_MAX_PART )
		max_part = GC_MULTIBOOT_MAX_PART;
	}
	else if ( mp->disc_type != disc->disc_type )
	    return ERROR0(ERR_SEMANTIC,
			"Combination of Wii and GameCube partitions is not possible.");

	if ( !mp->is_gc && qual_boot )
	    return ERROR0(ERR_SEMANTIC,
			"Qualifier 'boot' is only allowed for GameCube partitions.");

	//--- selection

	if (!qual_select) // [doc]
	{
	    if (!mp->is_gc)
	    {
		wd_append_select_item(&psel,WD_SM_DENY_PTAB,1,0);
		wd_append_select_item(&psel,WD_SM_DENY_PTAB,2,0);
		wd_append_select_item(&psel,WD_SM_DENY_PTAB,3,0);
		wd_append_select_item(&psel,WD_SM_ALLOW_PTYPE,0,WD_PART_DATA);
	    }
	    else if (qual_boot)
		wd_append_select_item(&psel,WD_SM_ALLOW_GC_BOOT,0,0);
	    else
		wd_append_select_item(&psel,WD_SM_DENY_GC_BOOT,0,0);
	}

	//wd_print_select(stdout,3,&psel);
	wd_select(disc,&psel);
	wd_reset_select(&psel);


	if (qual_header)
	    mp->source_dhead = &disc->dhead;
	if (qual_region)
	    mp->source_region = &disc->region;

	wd_part_t *part, *end_part = disc->part + disc->n_part;
	Mix_t * mix = 0;
	for ( part = disc->part; part < end_part; part++ )
	{
	    if (!part->is_enabled)
		continue;
	    err = wd_load_part(part,false,false,false);
	    if (err)
		return err;

	    if ( mp->n_mix >= max_part )
		return ERROR0(ERR_SEMANTIC,
			"Maximum supported partition count (%u) exceeded.\n",
			max_part );

	    if (qual_boot)
	    {
		ASSERT(!mp->have_boot_part);
		mp->have_boot_part = true;
		if (mp->n_mix)
		    memmove( mp->mix+1, mp->mix, mp->n_mix * sizeof(*mp->mix) );
		mp->n_mix++;
		mix = mp->mix;
		memset(mix,0,sizeof(*mix));
	    }
	    else
		mix = mp->mix + mp->n_mix++;

	    mix->source		= srcfile;
	    mix->ptab		= ptab_valid ? ptab : part->ptab_index;
	    mix->ptype		= ptype_valid ? ptype : part->part_type;
	    mix->trim		= trim;
	    mix->sf		= sf;
	    mix->have_pat	= pat != 0;
	    mix->disc		= disc;
	    mix->part		= part;
	    mix->src_sector	= part->part_off4 / WII_SECTOR_SIZE4;
	    ptab_count[ptab]++;


	    //--- find largest hole

	    if (pat)
		wd_select_part_files(part,IsFileSelected,pat);
	    wd_usage_t usage_id = part->usage_id;
	    u8 * utab = wd_calc_usage_table(mix->disc);

	    u32 src_sector = mix->src_sector;
	    u32 max_hole_off = 0, max_hole_size = 0;

	    for(;;)
	    {
		mix->n_blocks++;
		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
		    src_sector++;
		mix->src_end_sector = src_sector;

		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
		    src_sector++;

		if ( src_sector == part->end_sector )
		    break;

		const u32 hole_size = src_sector - mix->src_end_sector;
		if ( hole_size > max_hole_size )
		{
		    max_hole_size = hole_size;
		    max_hole_off  = mix->src_end_sector;
		}
	    }

	    if (max_hole_size)
	    {
		mix->a1size = max_hole_off - mix->src_sector;
		mix->a2off  = mix->a1size + max_hole_size;
		mix->a2size = mix->src_end_sector - (max_hole_off+max_hole_size);
	    }
	    else
	    {
		mix->a1size = mix->src_end_sector - mix->src_sector;
		mix->a2off  = 0;
		mix->a2size = 0;
	    }


	    PRINT("N-BLOCKS=%d, A1=0+%x, A2=%x+%x\n",
			mix->n_blocks, mix->a1size, mix->a2off, mix->a2size );
	}

	if (!mix)
	    return ERROR0(ERR_SEMANTIC,"No partition selected: %s\n",srcfile);
	mix->free_data = true;
    }

    if (!mp->n_mix)
	return ERROR0(ERR_SEMANTIC,"No source partition selected.\n");

    mp->end_mix = mp->mix + mp->n_mix;


    //---------------------------------------------------------------------
    //-----------		calculate start offsets		-----------
    //---------------------------------------------------------------------

    mp->is_multiboot = mp->is_gc && mp->n_mix > 1;
    mp->sector_align = wd_align_part(1,opt_align_part,mp->is_gc) / WII_SECTOR_SIZE;
    mp->start_sector = !mp->is_gc
			 ? WII_PART_OFF/WII_SECTOR_SIZE
			 : mp->have_boot_part || !mp->is_multiboot
				? 0
				: WII_SECTOR_SIZE/WII_SECTOR_SIZE;
			
    mp->start_sector = wd_align32(mp->start_sector,mp->sector_align,1);

    PRINT( "MIX START SECTOR = %x ==> off=%x, align-sector=%x,  n-mix=%d\n",
		mp->start_sector, mp->start_sector * WII_SECTOR_SIZE,
		mp->sector_align, mp->n_mix );

    u32 sector = mp->start_sector;
    Mix_t *mix, *end_mix = mp->mix + mp->n_mix;

    for ( mix = mp->mix; mix < end_mix; mix++ )
    {
	mix->dest_sector = sector;
	sector += mix->src_end_sector - mix->src_sector;
	sector = wd_align32(sector,mp->sector_align,1);
	PRINT("  - MIX #%02zu: %9llx .. %9llx -> %9llx .. %9llx\n",
		mix-mp->mix,
		WII_SECTOR_SIZE * (u64)mix->src_sector,
		WII_SECTOR_SIZE * (u64)mix->src_end_sector,
		WII_SECTOR_SIZE * (u64)mix->dest_sector,
		WII_SECTOR_SIZE * (u64)sector );
    }
    
    mp->max_sector = sector;

 #ifdef TEST
    verify_mix(mp,false,false);
 #endif

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			mark_used_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static u32 mark_used_mix
(
    MixFree_t	* base,		// pointer to free field
    u32		used,		// number of used elements in 'base'
    MixFree_t	* ptr,		// current pointer into 'base'
    u32		off,		// offset relative to ptr->off
    u32		size		// size to remove
) 
{
    DASSERT(base);
    DASSERT(used);
    DASSERT(ptr);
    DASSERT(size);
    noTRACE("MARK=%x+%x, ptr=%x+%x\n",off,size,ptr->off,ptr->size);
    ASSERT_MSG( off + size <= ptr->size,
		"off+size=%x+%x=%x <= ptr->size=%x\n",
		off, size, off+size, ptr->size );

    if (!off)
    {
	noTRACE("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off + size, ptr->off + ptr->size,
		(int)(ptr-base), used );
	ptr->off  += size;
	ptr->size -= size;
	if (!ptr->size)
	{
	    used--;
	    const u32 index = ptr - base;
	    memmove(ptr,ptr+1,(used-index)*sizeof(*ptr));
	}
	return used;
    }

    const u32 end = off + size;
    if ( end == ptr->size )
    {
	noTRACE("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off, ptr->off + ptr->size - size,
		(int)(ptr-base), used );
	ptr->size -= size;
	return used;
    }

    // split entry
    const u32 old_off  = ptr->off;
    const u32 old_size = ptr->size;

    const u32 index = ptr - base;
    memmove(ptr+1,ptr,(used-index)*sizeof(*ptr));

    ptr[0].off  = old_off;
    ptr[0].size = off;
    ptr[1].off  = old_off + end;
    ptr[1].size = old_size - end;
    noTRACE("\t\t\t\t\t> %x..%x -> %x..%x,%x..%x [%u/%u>%u]\n",
		old_off, old_off + old_size,
		ptr[0].off, ptr[0].off + ptr[0].size,
		ptr[1].off, ptr[1].off + ptr[1].size,
		(int)(ptr-base), used, used+1 );
    return used+1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			insert_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void insert_mix ( MixParam_t * mp, int pdepth, int mix_index )
{
    DASSERT( mp );
    DASSERT( pdepth >= 0 && pdepth < mp->n_mix );
    DASSERT( mix_index >= 0 && mix_index < mp->n_mix );
    
    MixFree_t * f = mp->field[pdepth];
    Mix_t * mix = mp->mix + mix_index;

    noTRACE("MIX#%u, a1=0+%x, a2=%x+%x\n",mix_index,mix->a1size,mix->a2off,mix->a2size);
    if (!pdepth)
    {
	// setup free table
	noTRACE("\t\t\t\t\t> SETUP\n");
	mp->delta[mix_index] = mp->start_sector;
	if (mix->a2off)
	{
	    mp->used[0]	= 2;
	    f[0].off	= mp->start_sector + mix->a1size;
	    f[0].size	= mix->a2off - mix->a1size;
	    f[1].off	= mp->start_sector + mix->a2off + mix->a2size;
	    f[1].size	= ~(u32)0 - f[1].off;
	}
	else
	{
	    mp->used[0]	= 1;
	    f[0].off	= mp->start_sector + mix->a1size;
	    f[0].size	= ~(u32)0 - f[0].off;
	}
    }
    else
    {
	u32 used = mp->used[pdepth-1];
	memcpy(f,mp->field[pdepth-1],sizeof(*f)*used);

	MixFree_t * f1ptr = f;
	MixFree_t * fend = f1ptr + used;
	for ( ; f1ptr < fend; f1ptr++ )
	{
	    if ( f1ptr->size < mix->a1size )
		continue;

	    if (!mix->a2off)
	    {
		mp->delta[mix_index] = f1ptr->off;
		used = mark_used_mix(f,used,f1ptr,0,mix->a1size);
		break;
	    }

	    const u32 base  = f1ptr->off;
	    const u32 space = f1ptr->size - mix->a1size;
	    MixFree_t * f2ptr;
	    for ( f2ptr = f1ptr; f2ptr < fend; f2ptr++ )
	    {
		u32 delta = 0;
		u32 off = base + mix->a2off;
		if ( f2ptr->off > off && f2ptr->off <= off + space )
		{
		    delta = f2ptr->off - off;
		    off += delta;
		}

		if ( off >= f2ptr->off && off + mix->a2size <= f2ptr->off + f2ptr->size )
		{
		    mp->delta[mix_index] = f1ptr->off + delta;
		    used = mark_used_mix(f,used,f2ptr,off-f2ptr->off,mix->a2size);
		    used = mark_used_mix(f,used,f1ptr,delta,mix->a1size);
		    f1ptr = fend;
		    break;
		}
	    }
	}

	mp->used[pdepth] = used;
    }

 #if defined(TEST) && defined(DEBUG) && 0
    {
	int i;
	for ( i = 0; i < mp->used[pdepth]; i++ )
	    printf(" %5x .. %8x / %8x\n",
		f[i].off, f[i].off + f[i].size, f[i].size ); 
    }
 #endif
}
 
//
///////////////////////////////////////////////////////////////////////////////
///////////////			permutate_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void permutate_mix ( MixParam_t * mp )
{
    PRINT("PERMUTATE MIX\n");

    const int n_perm = mp->n_mix;
    DASSERT( n_perm <= MAX_MIX_PERM );

    u8 used[MAX_MIX_PERM+1];	// mark source index as used
    memset(used,0,sizeof(used));
    u8 source[MAX_MIX_PERM];	// field with source index into 'mix'
    source[0] = 0;
    int pdepth = 0;
    mp->max_end = ~(u32)0;

    u32	delta[MAX_MIX_PERM];
    memset(delta,0,sizeof(delta));

    for(;;)
    {
	u8 src = source[pdepth];
	if ( src > 0 )
	    used[src] = 0;
	src++;
	while ( src <= n_perm && used[src] )
	    src++;
	if ( src > n_perm )
	{
	    if ( --pdepth >= 0 )
		continue;
	    break;
	}

	noTRACE(" src=%d/%d, depth=%d\n",src,n_perm,pdepth);
	DASSERT( src <= n_perm );
	used[src] = 1;
	source[pdepth] = src;
	insert_mix(mp,pdepth++,src-1);

	// [[2do]] : insert part

	if ( pdepth < n_perm )
	{
	    source[pdepth] = 0;
	    continue;
	}

	const u32 used = mp->used[pdepth-1];
	const u32 new_end = mp->field[pdepth-1][used-1].off;

     #if defined(TEST) && 0
	{
	    PRINT("PERM:");
	    int i;
	    for ( i = 0; i < n_perm; i++ )
		PRINT(" %u",source[i]);
	    PRINT(" -> %5x [%5x]%s\n",
		new_end, mp->max_end, new_end < mp->max_end ? " *" : "" );
	}
     #endif
     
	if ( new_end < mp->max_end )
	{
	    mp->max_end = new_end;
	    memcpy(delta,mp->delta,sizeof(delta));
	}
    }
    
    int i;
    for ( i = 0; i < n_perm; i++ )
	mp->mix[i].dest_sector = delta[i];
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup_sector_mix()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void setup_sector_mix ( Mix_t * mix )
{
    DASSERT(mix);
    DASSERT(mix->disc);
    DASSERT(mix->part);
    DASSERT(!mix->used_free);

    const int uf_count = 2 * mix->n_blocks + 1;
    u32 * uf = CALLOC(uf_count,sizeof(u32));
    mix->used_free = uf;

    wd_usage_t usage_id = mix->part->usage_id;
    u8 * utab = wd_calc_usage_table(mix->disc);
    u32 src_sector = mix->src_sector;
    u32 last_value = src_sector;

    while ( src_sector < mix->src_end_sector )
    {
	while ( src_sector < mix->src_end_sector
		&& ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
	    src_sector++;
	*uf++ = src_sector - last_value;
	last_value = src_sector;

	while ( src_sector < mix->src_end_sector
		&& ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
	    src_sector++;
	*uf++ = src_sector - last_value;
	last_value = src_sector;

	noTRACE("SETUP: %5x %5x\n",uf[-2],uf[-1]);
	ASSERT( uf < mix->used_free + uf_count );
    }
    
    ASSERT( uf+1 == mix->used_free + uf_count );
    uf[-1] = ~(u32)0 >> 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sector_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void sector_mix ( MixParam_t * mp, Mix_t * m1, Mix_t * m2 )
{
    DASSERT(mp);
    DASSERT(m1);
    DASSERT(m2);
    DASSERT(m1->used_free);
    DASSERT(m2->used_free);

    const u32 n_sect2 = m2->src_end_sector - m2->src_sector;
 #if DEBUG_SM
    const u32 max_delta = mp->max_end - n_sect2 + 1 + 0x1000;
 #else
    const u32 max_delta = mp->max_end - n_sect2 + 1;
 #endif

    u32 delta = *m1->used_free;

    for(;;)
    {
     restart:
	if ( delta >= max_delta )
	{
	    PRINT("ABORTED AT DELTA %x/%x\n",delta,max_delta);
	    return; // abort
	}

	PRINT("TRY DELTA %x/%x\n",delta,max_delta);

	u32 * uf1 = m1->used_free;
	u32 * uf2 = m2->used_free;
	u32 hole_size = delta;

	for(;;)
	{
	    //---- uf1

	    for(;;)
	    {
		PRINT_SM("\tuf1a=%8x,%8x, hole_size=%5x\n",uf1[0],uf1[1],hole_size);
		const u32 used_val = *uf1++;
		if (!used_val)
		    goto ok;
		if ( hole_size < used_val )
		{
		    delta += used_val - hole_size;
		    goto restart;
		}
		hole_size -= used_val;

		PRINT_SM("\tuf1b=%8x,%8x, hole_size=%5x\n",uf1[0],uf1[1],hole_size);
		const u32 free_val = *uf1++;
		DASSERT(free_val);
		if ( hole_size <= free_val )
		{
		    hole_size = free_val - hole_size;
		    break;
		}
		hole_size -= free_val;
	    }

	    //---- uf2

	    for(;;)
	    {
		PRINT_SM("\tuf2a=%8x,%8x, hole_size=%5x\n",uf2[0],uf2[1],hole_size);
		const u32 used_val = *uf2++;
		if (!used_val)
		    goto ok;
		if ( hole_size < used_val )
		{
		    delta += hole_size + *uf1;
		    goto restart;
		}
		hole_size -= used_val;

		PRINT_SM("\tuf2b=%8x,%8x, hole_size=%5x\n",uf2[0],uf2[1],hole_size);
		const u32 free_val = *uf2++;
		DASSERT(free_val);
		if ( hole_size <= free_val )
		{
		    hole_size = free_val - hole_size;
		    break;
		}
		hole_size -= free_val;
	    }
	}

	break;
    }
    
 ok:;
    const u32 max_end = delta + n_sect2;
    PRINT(">>> FOUND: delta=%x, max_end=%x %c %x\n",
		delta, max_end,
		max_end < mp->max_end ? '<' : max_end > mp->max_end ? '>' : '=',
		mp->max_end);

 #if !DEBUG_SM
    if ( max_end < mp->max_end )
 #endif
    {
	m1->dest_sector = mp->start_sector;
	m2->dest_sector = mp->start_sector + delta;
	mp->max_end = max_end;
     #ifdef TEST
	verify_mix(mp,false,false);
     #endif
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sector_mix_2()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void sector_mix_2 ( MixParam_t * mp )
{
    DASSERT(mp);
    DASSERT( mp->n_mix == 2 );
    
    setup_sector_mix(mp->mix);
    setup_sector_mix(mp->mix+1);

    PRINT("SECTOR MIX 0,1\n");
    sector_mix(mp,mp->mix,mp->mix+1);

    PRINT("SECTOR MIX 1,0\n");
    sector_mix(mp,mp->mix+1,mp->mix);
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			log_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void log_mix
(
    MixParam_t	* mp		// valid pointer to mix param + data
)
{
    DASSERT(mp);

    int src_fw = 13;
    Mix_t * mix;
    for ( mix = mp->mix; mix < mp->end_mix; mix++ )
    {
	const int len = strlen(mix->source);
	if ( src_fw < len )
	     src_fw = len;
    }

    printf("\nMix table (%d partitions, %s, total size=%llu MiB):\n\n",
		mp->n_mix, overlay_info[mp->overlay],
		mp->max_sector * (u64)WII_SECTOR_SIZE / MiB );

    if ( mp->overlay < MULTI_HOLE_OVERLAY )
    {
	printf("    blocks #1      blocks #2  :      disc offset     "
		": ptab  ptype : ignore + source\n"
		"  %.*s\n",
		69 + src_fw, wd_sep_200 );

	for ( mix = mp->mix; mix < mp->end_mix; mix++ )
	{
	    if (mix->a2off)
	    {
		const u32 end_sector = mix->dest_sector + mix->a2off + mix->a2size;
		printf("%7x..%5x + %5x..%5x : %9llx..%9llx : %u %s : %c %s\n",
		    mix->dest_sector,
		    mix->dest_sector + mix->a1size,
		    mix->dest_sector + mix->a2off,
		    end_sector,
		    mix->dest_sector * (u64)WII_SECTOR_SIZE,
		    end_sector * (u64)WII_SECTOR_SIZE,
		    mix->ptab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		    mix->have_pat ? '+' : '-', mix->source );
	    }
	    else
	    {
		const u32 end_sector = mix->dest_sector + mix->a1size;
		printf("%7x..%5x                : %9llx..%9llx : %u %s : %c %s\n",
		    mix->dest_sector, end_sector,
		    mix->dest_sector * (u64)WII_SECTOR_SIZE,
		    end_sector * (u64)WII_SECTOR_SIZE,
		    mix->ptab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		    mix->have_pat ? '+' : '-', mix->source );
	    }
	}
    }
    else
    {
	printf("  n(blks) :      disc offset     : ptab  ptype : ignore + source\n"
		"  %.*s\n",
		49 + src_fw, wd_sep_200 );

	for ( mix = mp->mix; mix < mp->end_mix; mix++ )
	{
	    const u32 end_sector = mix->dest_sector + mix->a2off + mix->a2size;
	    printf("%9u : %9llx..%9llx : %u %s : %c %s\n",
		mix->n_blocks,
		mix->dest_sector * (u64)WII_SECTOR_SIZE,
		end_sector * (u64)WII_SECTOR_SIZE,
		mix->ptab,
		wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		mix->have_pat ? '+' : '-', mix->source );
	}
    }
    putchar('\n');
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			create_output_image()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError create_output_image
(
    MixParam_t	* mp		// valid pointer to mix param + data
)
{
    enumError err = CreateWFile( &mp->fo.f, 0, IOM_IS_IMAGE,
					InfoUI_wit.opt_used[OPT_OVERWRITE] ? 1 : 0 );
    if (err)
	return err;

    if (opt_split)
	SetupSplitWFile(&mp->fo.f,mp->fo.iod.oft,opt_split_size);

    err = SetupWriteSF(&mp->fo,mp->fo.iod.oft);
    if (err)
	return err;

    return SetMinSizeSF( &mp->fo, opt_disc_size ? opt_disc_size : mp->dest_file_size );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			copy_partitions()		///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError copy_partitions
(
    MixParam_t	* mp		// valid pointer to mix param + data
)
{
    DASSERT(mp);

    //----- copy data

    int mi;
    for ( mi = 0; mi < mp->mm.used; mi++ )
    {
	MemMapItem_t * item = mp->mm.field[mi];
	DASSERT(item);
	u32 index = item->index & 0x7f;
	DASSERT( index < mp->n_mix );
	Mix_t * mix = mp->mix + index;
	u64 src_off = item->off
		    + ( (int)mix->src_sector - (int)mix->dest_sector )
		    * (s64)WII_SECTOR_SIZE;
	if ( verbose > 0 )
	    printf(" - copy P.%-2u %9llx -> %9llx .. %9llx, size=%9llx\n",
		index, src_off, (u64)item->off,
		(u64)item->off + (u64)item->size, (u64)item->size );

	const enumError err
	    = CopyRawData2(mix->sf,src_off,&mp->fo,item->off,item->size);
	if (err)
	    return err;
    }


    //----- rewrite modified data

    for ( mi = 0; mi < mp->n_mix; mi++ )
    {
	Mix_t * mix = mp->mix + mi;
	u64 delta_off = ( (int)mix->dest_sector - (int)mix->src_sector )
			* (s64)WII_SECTOR_SIZE;
	PRINT("-> REWRITE #%02u: %16llx\n",mi,delta_off);

	const enumError err
	    = RewriteModifiedSF(mix->sf,&mp->fo,0,delta_off);
	if (err)
	    return err;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			create_gc_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError create_gc_mix
(
    MixParam_t	* mp,		// valid pointer to mix param + data
    wd_header_t * dhead,	// valid pointer to prepared disc header
    wd_region_t * reg		// valid pointer to prepared region set
)
{
    DASSERT(mp);
    DASSERT(mp->is_gc);
    DASSERT(dhead);
    DASSERT(reg);

    ccp disc_info1, disc_info2;

    char head[WII_SECTOR_SIZE];
    memset(head,0,sizeof(head));

    if (!mp->is_multiboot)
    {
	disc_info1 = "Single GameCube partition.";
	disc_info2 = "";
	DASSERT( sizeof(head) >= sizeof(*dhead) );
	memcpy(head,dhead,sizeof(*dhead));
	memset(head+GC_MULTIBOOT_PTAB_OFF,0,GC_MULTIBOOT_PTAB_SIZE);
    }
    else
    {
	if (mp->have_boot_part)
	{
	    Mix_t * mix = mp->mix;
	    DASSERT(mix->is_boot_part);
	    disc_info2 = " with boot partition.";
	    const enumError err = ReadSF(mix->sf,0,head,sizeof(head));
	    if (err)
		return err;
	}
	else
	{
	    disc_info2 = " without boot partition.";
	    header_setup((wd_header_t*)head,0,0,true);
	}
	memcpy(head + WII_TITLE_OFF, dhead->disc_title, WII_TITLE_SIZE );
	memset(head+GC_MULTIBOOT_PTAB_OFF,0,GC_MULTIBOOT_PTAB_SIZE);

	// check dvd9
	u32 max_src_sector = 0;
	Mix_t * mix;
	for ( mix = mp->mix; mix < mp->end_mix; mix++ )
	    if ( max_src_sector < mix->src_sector )
		 max_src_sector = mix->src_sector;
	const bool is_dvd9 = max_src_sector >= WII_MAX_U32_SECTORS;
	noPRINT("DVD9=%d, WII_MAX_U32_SECTORS=%x\n",is_dvd9,WII_MAX_U32_SECTORS);
	disc_info1 = is_dvd9
			? "Multiboot GameCube disc, format=DVD9"
			: "Multiboot GameCube disc";

	u32 * ptab = (u32*)( head + GC_MULTIBOOT_PTAB_OFF );
	memcpy( head, is_dvd9 ? "GCOPDVD9" : "GCOPDVD", 8 );
	memset(ptab,0,GC_MULTIBOOT_PTAB_SIZE);

	for ( mix = mp->mix; mix < mp->end_mix; mix++ )
	    if (!mix->is_boot_part)
	    {
		u32 off = mix->src_sector * WII_SECTOR_SIZE4;
		if (!is_dvd9)
		    off <<= 2;
		*ptab++ = htonl(off);
		DASSERT( (ccp)ptab <= head + GC_MULTIBOOT_PTAB_OFF + GC_MULTIBOOT_PTAB_SIZE );
	    }
    }
    *(u32*)( head + WII_BI2_OFF + WII_BI2_REGION_OFF ) = reg->region;

    const enumOFT oft = mp->fo.iod.oft;

    if ( testmode || verbose >= 0 )
    {
	const u32 regnum = ntohl(reg->region);
	printf("\n%sreate disc [%.6s] %s:%s\n"
		"  %s%s\n"
		"  Title:  %.64s\n"
		"  Region: %x [%s]\n\n",
		testmode ? "WOULD c" : "C",
		head, oft_info[oft].name, mp->fo.f.fname,
		disc_info1, disc_info2,
		head + WII_TITLE_OFF,
		regnum, GetRegionName(regnum,"?") );
    }

    if (testmode)
	return ERR_OK;

 #ifndef TEST // {2do]
    if ( verbose >= 0 )
	ERROR0(ERR_WARNING,
		"***********************************************\n"
		"***  The GameCube support is EXPERIMENTAL!  ***\n"
		"***********************************************\n" );
 #endif


    //--- create file

    enumError err = create_output_image(mp);
    if (err)
	return err;


    //--- write disc header

    err = WriteSparseSF(&mp->fo,0,head,sizeof(head));
    if (err)
	return err;


    //--- copy partitions

    return copy_partitions(mp);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			create_wii_mix()		///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError create_wii_mix
(
    MixParam_t	* mp,		// valid pointer to mix param + data
    wd_header_t * dhead,	// valid pointer to prepared disc header
    wd_region_t * reg		// valid pointer to prepared region set
)
{
    DASSERT(mp);
    DASSERT(!mp->is_gc);
    DASSERT(dhead);
    DASSERT(reg);

    const enumOFT oft = mp->fo.iod.oft;

    if ( testmode || verbose >= 0 )
    {
	const u32 regnum = ntohl(reg->region);
	printf("\n%sreate Wii disc [%.6s] %s:%s\n"
		"  Title:  %.64s\n"
		"  Region: %x [%s] / %s\n\n",
		testmode ? "WOULD c" : "C",
		&dhead->disc_id, oft_info[oft].name,
		mp->fo.f.fname, dhead->disc_title,
		regnum, GetRegionName(regnum,"?"),
		wd_print_age_rating(0,0,reg->age_rating) );
    }

    if (testmode)
	return ERR_OK;


    //--- create file

    enumError err = create_output_image(mp);
    if (err)
	return err;


    //--- write disc header

    err = WriteSparseSF(&mp->fo,0,dhead,sizeof(*dhead));
    if (err)
	return err;


    //--- write partition tables

    wd_ptab_t ptab;
    memset(&ptab,0,sizeof(ptab));
    {
	wd_ptab_info_t  * info  = ptab.info;
	wd_ptab_entry_t * entry = ptab.entry;
	u32 off4 = (ccp)entry - (ccp)info + WII_PTAB_REF_OFF >> 2;

	int it;
	for ( it = 0; it < WII_MAX_PTAB; it++, info++ )
	{
	    int n_part = 0;
	    Mix_t * mix;
	    for ( mix = mp->mix; mix < mp->end_mix; mix++ )
	    {
		if ( mix->ptab != it )
		    continue;

		n_part++;
		entry->off4  = htonl(mix->dest_sector*WII_SECTOR_SIZE4);
		entry->ptype = htonl(mix->ptype);
		entry++;
	    }

	    if (n_part)
	    {
		info->n_part = htonl(n_part);
		info->off4   = htonl(off4);
		off4 += n_part * sizeof(*entry) >> 2;
	    }
	}
    }

    err = WriteSparseSF(&mp->fo,WII_PTAB_REF_OFF,&ptab,sizeof(ptab));
    if (err)
	return err;


    //--- write region settings

    err = WriteSF(&mp->fo,WII_REGION_OFF,reg,sizeof(*reg));
    if (err)
	return err;


    //--- write magic2

    u32 magic2 = htonl(WII_MAGIC2);
    err = WriteSF(&mp->fo,WII_MAGIC2_OFF,&magic2,sizeof(magic2));
    if (err)
	return err;


    //--- copy partitions

    return copy_partitions(mp);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			cmd_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_mix()
{
    TRACE_SIZEOF(Mix_t);
    TRACE_SIZEOF(MixParam_t);
    TRACE_SIZEOF(MixParam_t);

    opt_hook = -1; // disable hooked discs (no patching/relocation)
 
    if ( verbose >= 0 )
	print_title(stdout);

    //----- check dest option

    if ( !opt_dest || !*opt_dest )
    {
	if (!testmode)
	    return ERROR0(ERR_SEMANTIC,
			"Non empty option --dest/--DEST required.\n");
	opt_dest = "./";
    }


    //---------------------------------------------------------------------
    //-----------		scan parameters			-----------
    //---------------------------------------------------------------------

    MixParam_t mp;
    memset(&mp,0,sizeof(mp));
    InitializeSF(&mp.fo);
    InitializeMemMap(&mp.mm);
    mp.overlay = NO_OVERLAY;

    enumError err = scan_mix_param(&mp);
    if (err)
	return err;


    //---------------------------------------------------------------------
    //-----------		overlay calculations		-----------
    //---------------------------------------------------------------------

    if ( mp.n_mix > 1 && InfoUI_wit.opt_used[OPT_OVERLAY] )
    {
	permutate_mix(&mp);
	if ( mp.max_sector > mp.max_end )
	{
	    PRINT("*** PERMUTATE => END SECTOR = %x < %x\n",
			mp.max_end, mp.max_sector );
	    mp.max_sector = mp.max_end;
	    mp.overlay = SIMPLE_OVERLAY;
	}

	if ( mp.n_mix == 2 )
	{   
	    sector_mix_2(&mp);
	    if ( mp.max_sector > mp.max_end )
	    {
		PRINT("*** SECTOR MIX 2 -> END SECTOR = %x < %x\n",
				mp.max_end, mp.max_sector );
		mp.max_sector = mp.max_end;
		mp.overlay = MULTI_HOLE_OVERLAY;
	    }
	}
    }


    //----- check max sectors

    if ( mp.max_sector > WII_MAX_SECTORS )
	return ERROR0(ERR_SEMANTIC,
		"Total size (%u MiB) exceeds maximum size (%u MiB).\n",
		mp.max_sector / WII_SECTORS_PER_MIB,
		WII_MAX_SECTORS / WII_SECTORS_PER_MIB );


    //---------------------------------------------------------------------
    //-----------		      logging			-----------
    //---------------------------------------------------------------------

    err = verify_mix( &mp, testmode > 1 || verbose > 2, true );

    if ( err || testmode || verbose > 1 )
    {
	log_mix(&mp);
	if (err)
	    return err;
    }

    if (logging)
    {
	printf("Partition layout of new mixed disc:\n\n");
	PrintMemMap(&mp.mm,stdout,3,0);
	putchar('\n');
    }


    //---------------------------------------------------------------------
    //-----------		    create file			-----------
    //---------------------------------------------------------------------


    //----- setup dhead

    char * dest = iobuf + sprintf(iobuf,"WIT mix of");
    ccp sep = " ";

    u64 dest_file_size = mp.is_gc
			    ? 0
			    : WII_SECTORS_SINGLE_LAYER * (u64)WII_SECTOR_SIZE;

    Mix_t * mix;
    for ( mix = mp.mix; mix < mp.end_mix; mix++ )
    {
	dest += sprintf(dest,"%s%s",sep,wd_print_id(&mix->part->boot.dhead.disc_id,6,0));
	sep = "+";
	const u64 end_off = mix->part->part_size + mix->dest_sector * (u64)WII_SECTOR_SIZE;
	if ( dest_file_size < end_off )
	     dest_file_size = end_off;
	PRINT("FILE-SIZE: %9llx %9llx\n",end_off,dest_file_size);
    }
    mp.dest_file_size = dest_file_size;
    if ( dest - iobuf >= ( mp.is_gc ? GC_MULTIBOOT_PTAB_OFF-WII_TITLE_OFF : WII_TITLE_SIZE ))
	sprintf(iobuf,"WIT mix of %u discs",mp.n_mix);

    wd_header_t dhead;
    if (mp.source_dhead)
    {
	memcpy(&dhead,mp.source_dhead,sizeof(dhead));
	PatchId(&dhead,modify_disc_id,0,6);
    }
    else if ( mp.n_mix == 1 )
	memcpy(&dhead,&mp.mix->disc->dhead,sizeof(dhead));
    else
	header_setup(&dhead,modify_id /* [[id]] */,iobuf,mp.is_gc);
    PatchName(dhead.disc_title,WD_MODIFY__ALWAYS);


    //----- setup region

    wd_region_t reg;
    memset(&reg,0,sizeof(reg));

    if ( opt_region < REGION__AUTO )
	reg.region = htonl(opt_region);
    else if ( mp.source_region )
	memcpy(&reg,mp.source_region,sizeof(reg));
    else if ( mp.n_mix == 1 )
	memcpy(&reg,&mp.mix->disc->region,sizeof(reg));
    else
	reg.region = htonl(GetRegionInfo(dhead.region_code)->reg);


    //----- setup output file
    
    ccp destfile = IsDirectory(opt_dest,true) ? "a.wdf" : "";
    const enumOFT oft = CalcOFT(output_file_type,opt_dest,destfile,OFT__DEFAULT);
    mp.fo.f.create_directory = opt_mkdir;
    GenImageWFileName(&mp.fo.f,opt_dest,destfile,oft);
    SetupIOD(&mp.fo,oft,oft);


    //----- create file

    err = mp.is_gc
	    ? create_gc_mix (&mp,&dhead,&reg)
	    : create_wii_mix(&mp,&dhead,&reg);
    if (err)
	RemoveSF(&mp.fo);


    //---------------------------------------------------------------------
    //-----------		    cleanup			-----------
    //---------------------------------------------------------------------

    ResetMemMap(&mp.mm);

    for ( mix = mp.mix; mix < mp.end_mix; mix++ )
    {
	FREE(mix->used_free);
	if (mix->free_data)
	    FreeSF(mix->sf);
    }

    enumError err2 = ResetSF(&mp.fo,0);
    return err ? err : err2;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

