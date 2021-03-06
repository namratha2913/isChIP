#pragma once

#include "TxtFile.h"
#include <algorithm>    // std::sort

#ifndef _NO_UNODMAP
#ifdef OS_Windows
#include <unordered_map>
#else
#include <tr1/unordered_map>
using namespace std::tr1;
#endif
#endif	// _NO_UNODMAP

#define	CID(it)	(it)->first
#define	TREATED(it)	(it)->second.Treated

typedef pair<chrlen, chrlen> dchrlen;	// double chrlen

class ChromSizes;

// Basic class for objects keeping in Tab File
class Obj
{
public:
	enum eInfo {	// defines types of outputted info
		iNONE,	// nothing printed: it is never pointed in command line
		iLAC,	// laconic:		print file name if needs
		iNM,	// name:		print file name
		iEXT,	// standard:	print file name and items number
		iSTAT,	// statistics:	print file name, items number and statistics
	};

private:
	// Throws exception or warning message
	//	@err: input error
	//	@abortInvalid: if true, throws exception, otherwise throws warning
	void ThrowError(Err &err, bool abortInvalid);

public:
		// 'Ambig' handles items ambiguities and represents ambiguities statistics
	class Ambig
	{
	public:
		// Enum 'eCase' defines all possible ambiguous cases.
		// Check _CasesCnt value
		enum eCase	{
			DUPL,		// duplicated features
			CROSS,		// crossed features
			ADJAC,		// adjacent features
			COVER,		// coverage feature by another
			SHORT,		// too short features
			DIFFSZ,		// different size of reads
			SCORE,		// filtered by score features
			EXCEED,		// start or stop position exceeded chrom length
			NEGL		// belong to negligible chromosome
		};

		// Enum 'eAction' defines all possible reactions for ambiguous.
		enum eAction /*:BYTE*/ {	// commented type doesn't compiled under Linux GNU
			ACCEPT,			// accept ambiguous feature/read "as is", without treatment
			HANDLE,			// handle ambiguous feature/read
			OMIT,			// omit ambiguous feature/read with alarm warning
			OMIT_SILENT,	// omit ambiguous feature/read without alarm warning
			ABORTING		// abort execution via exception
		};

	private:
		// pointer to reaction
		typedef int	(Obj::Ambig::*ReportCase)(eCase ambig);

		struct Msg {
			const char*	TotalAlarm;	// supplementary message, added to case message in statistics
			const char*	StatInfo;	// short text of ambiguous used in statistics
			const string LineAlarm;	// text of ambiguous used in line alarm
		};
		static Msg	_Msgs[];				// texts of all ambiguous
		static const char*	_ActionMsgs[];	// messages followed by reactions
		static const ReportCase	_Actions[];	// reactions
		static const BYTE	_CasesCnt = 9;	// count of cases of feature/read ambiguities
	
		struct Case {
			BYTE Type;
			chrlen Count;
		};
		Case	_cases[_CasesCnt];
		TabFile*	 _file;			// current reading file
		const	FT::eTypes _fType;	// type of data
		const	eInfo _info;		// input type of info
		mutable chrlen _count;		// count of discovered ambiguous
		const	bool _alarm;		// true if message should be printed
		mutable bool _alarmPrinted;	// true if warning was printed
#ifndef _ISCHIP
		short	_treatcID;			// treated chrom ID: -1 initial, cID if only one chrom was treated.
									// UnID if more then one chrom was treated
#endif
		// ***** treatment
		inline int Accept(eCase ambg)	{ return 1; }
		inline int Handle(eCase ambg)	{ PrintLineAlarm(ambg); return 0; }
		inline int Omit  (eCase ambg)	{ PrintLineAlarm(ambg); return -1; }
		inline int OmitQuiet(eCase ambg){ return -1; }
		inline int Abort (eCase ambg)	{ ThrowExcept(_Msgs[ambg].LineAlarm); return -1; }

		// Get treatment message 
		const inline char* Message(eCase ambig) const { return _ActionMsgs[_cases[ambig].Type]; }
		
		inline const string& EntityName(chrlen cnt = 1) const { return FT::ItemTitle(_fType, cnt!=1); }

		// Throws exception with given code, contained file name (if needed)
		inline const void ThrowExcept (Err::eCode code)	const {	_file->ThrowLineExcept(code); }

		// Throws exception with given message, contained file name (if needed)
		inline const void ThrowExcept (const string& msg) const { _file->ThrowLineExcept(msg); }

		// Gets count of ambiguities
		chrlen Count() const;

		// Prints entities count
		//	@cID: readed chromosome's ID or Chrom::UnID if all
		//	@cnt: number of entity
		void PrintEntityCount(chrid cID, chrlen cnt) const;

		// Print given ambiguity as alarm
		//	@ambig: given ambiguity
		void PrintLineAlarm(eCase ambig) const;

		// Prints case statistics
		//	@ambig: ambiguity's case
		//	@allCnt: total count of ambiguities
		//	@total: if true then prints total warning case
		void PrintCaseStat(eCase ambig, chrlen allCnt, bool total=false) const;

		// Prints items with specifying chrom
		//	@cID: readed chromosome's ID or Chrom::UnID if all
		//	@prAcceptItems: if true then prints number of accepted items
		//	@itemCnt: count of accepted items after treatment
		void PrintItems(chrid cID, bool prAcceptItems, long itemCnt) const;

	public:
		bool unsortedItems;		// true if items are unsorted

		// Creates an instance with omitted COVER, SHORT, SCORE and NEGL cases;
		// BedF cases by default:
		// omitted DUPL cases and handled CROSS and ADJAC 
		Ambig (eInfo info, bool alarm, FT::eTypes format,
			eAction dupl = OMIT,
			eAction crossANDadjac = HANDLE,
			eAction diffsz = ACCEPT	// in fact for BedF it even doesn't check
		);

		// Sets current Tab File
		inline void SetFile (TabFile& file) { _file = &file; }
		
		// Gets current Tab File
		inline TabFile& File () const		{ return *_file; }

		inline FT::eTypes FileType() const	{ return _fType; }

		inline bool IsAlarmPrinted() const	{ return _alarmPrinted; }
#ifndef _ISCHIP
		// Remember treated chrom.
		void	SetTreatedChrom(chrid cid);

		// Gets treated chrom: cID if only one chrom was treated, UnID otherwise.
		void KeepTreatedChrom() const { if(_treatcID != vUNDEF)	Chrom::SetStatedID(chrid(_treatcID)); }
#endif
		// Initializes given Region by second and third current reading line positions, with validating
		//	@rgn: Region that should be initialized
		//	@cLen: chrom lemgth
		//	return: true if Region was initialized successfully
		bool InitRegn(Region& rgn, chrlen cLen);

		// Adds statistics and print given ambiguity as alarm (if permitted)
		//	@ambig: given ambiguity
		//	return: treatment code: 1 - accept, 0 - handle, -1 - omit
		int TreatCase(eCase ambig);

		// Prints statistics.
		//	@cID: readed chromosome's ID or Chrom::UnID if all
		//	@title: string at the beginning; if NULL then this instance is used while initialization
		//	and don't feeds line
		//	@totalItemCnt: count of all items
		//	@acceptItemCnt: count of accepted items after treatment
		//	return: true if EOL has been printed
		bool Print(chrid cID, const char* title, ULONG totalItemCnt, ULONG acceptItemCnt);

		// Sets supplementary message, added to case message in statistics
		//	@ambig: given case
		//	@msg: supplementary message
		static inline void SetSupplAlarm(eCase ambig, const char* msg) {
			_Msgs[ambig].TotalAlarm = msg;
		}
	};	//***** end of class Ambig

	bool _isBad;		// sign of invalidity
	bool _EOLneeded;	// true if EOL 

	inline Obj() : _isBad(false), _EOLneeded(false) {}

	// Initializes new instance by tab file name.
	//	@title: title printed before file name
	//	@fName: name of file
	//	@ambig: ambiguities
	//	@addObj: auxiliary object using while initializing
	//	@isInfo: true if file info hpuld be printed
	//	@abortInvalid: true if invalid instance shold be completed by throwing exception
	void Init	(const char* title, const string& fName, Ambig& ambig, void* addObj,
		bool isInfo, bool abortInvalid);

	// Initializes child instance from tab file
	//	@ambig: ambiguities
	//	@addObj: auxiliary object
	//	return: numbers of all and initialied items for given chrom
	virtual dchrlen InitChild	(Ambig& ambig, void* addObj) = 0;

	// Gets item's title.
	//	@pl: true if plural form
	virtual const string & ItemTitle(bool pl=false) const = 0;

	// Prints EOL if needs.
	//	@printEOL: true if EOL should be printed
	void PrintEOL(bool printEOL);

public:
	// Returns true if instance is invalid.
	inline bool IsBad()	const { return _isBad; }

	// Returns true if something was printed during initialization without EOL
	inline bool EOLNeeded() const { return _EOLneeded; }
};

static const string range_out_msg = "Chroms[]: invalid chrom ID ";

// Basic class for all chromosomes collection,
template <typename T> class Chroms
{
protected:
#ifdef _NO_UNODMAP
	typedef pair<chrid,T> chrItem;
	typedef vector<chrItem> chrItems;

	static inline bool Compare (const chrItem& ci1, const chrItem& ci2) {
		return ci1.first < ci2.first;
	}
#else
	typedef unordered_map<chrid, T> chrItems;
#endif	// _NO_UNODMAP

private:
	chrItems _chroms;	// storage of chromosomes

public:
	typedef typename chrItems::iterator Iter;			// iterator
	typedef typename chrItems::const_iterator cIter;	// constant iterator

	// Returns a random-access constant iterator to the first element in the container
	inline cIter cBegin() const { return _chroms.begin(); }
	// Returns the past-the-end constant iterator.
	inline cIter cEnd()	  const { return _chroms.end(); }

	// Returns a random-access iterator to the first element in the container
	inline Iter Begin()	{ return _chroms.begin(); }
	// Returns the past-the-end iterator.
	inline Iter End()	{ return _chroms.end(); }

protected:
	// Reserves container's capacity
	inline void Reserve (chrid cCnt) {
		if( cCnt > 1 )
		#ifdef _NO_UNODMAP
			_chroms.reserve(cCnt);
		#else
			_chroms.rehash(cCnt);
		#endif
	}

	// Returns reference to the chromosome's item at its ID
	inline const T & At(chrid cID) const {
	#ifdef _NO_UNODMAP
		cIter it = GetIter(cID);
		if( it == cEnd() )
			throw std::out_of_range (range_out_msg + BSTR(cID));
		return it->second;
	#else
		return _chroms.at(cID);
	#endif	// _NO_UNODMAP
	}

	inline T & At(chrid cID) {
	#ifdef _NO_UNODMAP
		Iter it = GetIter(cID);
		if( it == End() )
			throw std::out_of_range (range_out_msg + BSTR(cID));
		return it->second;
	#else
		return _chroms.at(cID);
		//return _chroms[cID];	// inserts a new element if cID does not match the key of any element
	#endif	// _NO_UNODMAP
	}

	// Sorts container if possible
	inline void Sort() {
	#ifdef _NO_UNODMAP
		sort(Begin(), End(), Compare);
	#endif	// _NO_UNODMAP
	}

	// Adds empty class type to the collection without checking cID
	//	return: class type collection reference
	T& AddEmptyClass(chrid cID) {
	#ifdef _NO_UNODMAP
		_chroms.push_back( chrItem(cID, T()) );
		return (_chroms.end()-1)->second;
	#else
		return _chroms[cID];
	#endif	// _NO_UNODMAP
	}

	// Adds class type to the collection without checking cID.
	// Avoids unnecessery copy constructor call
	//	return: class type collection reference
	inline const T & AddClass(chrid cID, const T & val) {
		return AddEmptyClass(cID) = val;
	}

public:
	// Searches the container for an chromosome cID and returns an iterator to it if found,
	// otherwise it returns an iterator to end (the element past the end of the container)
	Iter GetIter(chrid cID) {
	#ifdef _NO_UNODMAP
		for(Iter it = Begin(); it != End(); it++)
			if( it->first == cID )	return it;
		return End();
	#else
		return _chroms.find(cID);
	#endif	// _NO_UNODMAP
	}

	// Searches the container for an chromosome cID and returns a constant iterator to it if found,
	// otherwise it returns an iterator to cEnd (the element past the end of the container)
	const cIter GetIter(chrid cID) const {
	#ifdef _NO_UNODMAP
		for(cIter it = cBegin(); it != cEnd(); it++)
			if( it->first == cID )	return it;
		return cEnd();
	#else
		return _chroms.find(cID);
	#endif	// _NO_UNODMAP
	}

	// Returns count of chromosomes.
	inline chrid ChromsCount () const { return _chroms.size(); }

	// Adds value type to the collection without checking cID
	inline void AddVal(chrid cID, const T & val) {
	#ifdef _NO_UNODMAP
		_chroms.push_back( chrItem(cID, val) );
	#else
		_chroms[cID] = val;
	#endif	// _NO_UNODMAP
	}

	// Clear content
	inline void Clear() {
		_chroms.clear();
	//#ifdef _NO_UNODMAP
	//	_chroms.clear();
	//	_chroms.push_back( chrItem(cID, val) );
	//#else
	//	_chroms.clear();
	//#endif	// _NO_UNODMAP
	}

	// Insert value type to the collection: adds new value or replaces existed
	//void InsertVal(chrid cID, const T & val) {
	//#ifdef _NO_UNODMAP
	//	for(Iter it = Begin(); it != End(); it++)
	//		if( it->first == cID ) {
	//			it->second = val;
	//			return;
	//		}
	//	_chroms.push_back( chrItem(cID, val) );
	//#else
	//	_chroms[cID] = val;
	//#endif	// _NO_UNODMAP
	//}

	// Insert class type to the collection: adds new value or replaces existed
	//	returns: iterator to the value
	//Iter InsertClass(chrid cID, const T & val) {
	//#ifdef _NO_UNODMAP
	//	for(Iter it = Begin(); it != End(); it++)
	//		if( it->first == cID ) {
	//			it->second = val;
	//			return it;
	//		}
	//	//_chroms.push_back( chrItem(cID, val) );
	//	_chroms.push_back( chrItem(cID, T()) );	// alloc memory using default constructor
	//	return _chroms.end()-1;	// insert class member
	//#else
	//	_chroms[cID] = val;
	//#endif	// _NO_UNODMAP
	//}
	
	// Returns true if chromosome cID exists in the container, and false otherwise.
	bool FindChrom	(chrid cID) const {
	#ifdef _NO_UNODMAP
		for(cIter it = cBegin(); it != cEnd(); it++)
			if( it->first == cID )	
				return true;
		return false;
	#else
		return _chroms.count(cID) > 0;
	#endif	// _NO_UNODMAP
	}

	// Sets common chromosomes as 'Treated' in both of this instance and given Chroms.
	// Objects in both collections have to have second field as Treated.
	//	@obj: compared Chroms object
	//	@printWarn: if true print warning - uncommon chromosomes
	//	@throwExcept: if true then throw exception if no common chroms finded, otherwise print warning
	//	return: count of common chromosomes
	chrid	SetCommonChroms(Chroms<T>& obj, bool printWarn, bool throwExcept)
	{
		Iter it;
		chrid commCnt = 0;

		// set treated chroms in this instance
		for(it = Begin(); it != End(); it++)
			if( TREATED(it) = obj.FindChrom(CID(it)) )
				commCnt++;
			else if( printWarn )
				Err(Chrom::Absent(CID(it), "second file")).Warning();
		// set false treated chroms in a compared object
		for(it = obj.Begin(); it != obj.End(); it++)
			if( !FindChrom(CID(it)) ) {
				TREATED(it) = false;
				if( printWarn )
					Err(Chrom::Absent(CID(it), "first file")).Warning();
			}
		if( !commCnt )	Err("no common chromosomes").Throw(throwExcept);
		return commCnt;
	}
};

// 'ChromItemsInd' representes a range of chromosome's features/reads indexes,
struct ChromItemsInd
{
	bool	Treated;
	chrlen	FirstInd;	// first index in Feature's/Read's container
	chrlen	LastInd;	// last index in Feature's/Read's container

	inline ChromItemsInd(chrlen firstInd=0, chrlen lastInd=1)
		: Treated(true), FirstInd(firstInd), LastInd(lastInd-1) {}
		
	// Returns count of items
	inline size_t ItemsCount() const { return LastInd - FirstInd + 1; }
};

class Bed : public Obj, public Chroms<ChromItemsInd>
/*
 * Basic abstract class 'Bed' implements methods for creating list of chromosomes from bed-file.
 * Container of features/reads is complemented in derived class.
 * Skips comments in bed-file while reading.
 * strongly needs keyword 'abstract' but it doesn't compiled by GNU g++
 */
{
protected:
	static const BYTE _FieldsCnt = 6;	// count of fields readed from file

public:
	// Initializes instance from tab file
	//	@ambig: ambiguities
	//	@cSizes: chrom sizes
	//	return: numbers of all and initialied items for given chrom
	dchrlen InitChild	(Ambig& ambig, void* cSizes);
	 
	// Initializes size of positions container.
	virtual void ReserveItemContainer(ULONG initSize) = 0;

	 // Shrink size of positions container.
	virtual void ShrinkItemContainer() = 0;

	// Checks the last element for the new potential start/end positions for all possible ambiguous.
	//	@rgn: checked start/stop positions
	//	@ambig: possible ambiguities
	//  return: false if some ambiguous has found; true if alright
	virtual bool CheckLastPos(const Region& rgn, Ambig& ambig) = 0;

	// Adds Read to the container.
	//	@rgn: Region with mandatory fields
	//	@file: file to access to additionally fields
	//	return: true if Read was added successfully
	virtual bool AddPos(const Region& rgn, TabFile& file) = 0;

	// Sorts and rechecks items for the ambiguities.
	//	@ambig:  ambiguous filters
	virtual void SortItems(Ambig& ambig) = 0;

	// Sets and returns count of all Features/Reads.
	virtual void SetAllItemsCount() = 0;

	// Gets count of Features/Reads for chromosome
	virtual size_t ItemsCount(chrid cID) const = 0;

	// Gets count of all Features/Reads.
	virtual size_t AllItemsCount() const = 0;

	// Decreases Feature/Read position without checkup indexes.
	// Used in Shrink() method only.
	//	@cID: chromosome's ID
	//	@eInd: index of read
	//	@shift: decrease entry's start position on this value
	//	@regnEnd: region's end position to control Read location
	//	@return: true if Feature/Read is insinde this region
	//virtual bool DecreasePos(chrid cID, chrlen eInd, chrlen shift, chrlen regnEnd) = 0;

#ifdef DEBUG
	virtual void PrintItem(chrlen itemInd) const = 0;
#endif

public:
	inline const ChromItemsInd & operator[] (chrid cID) const { return At(cID); }

//#ifdef _DENPRO
//	// Shifts elements's positions to collaps the 'holes' between external regions.
//	//	@cID: chromosome's ID
//	//	@regns: external (defined) regions
//	void ShrinkByID(chrid cID, const Regions &regns);
//#endif	// _DENPRO

#ifdef DEBUG
	void PrintChrom() const;
	// Prints Bed with limited or all items
	//	@itemCnt: first number of items for each chromosome; all by default
	void Print(chrlen itemCnt=0) const;
#endif
};

template <typename I> class BedType	: public Bed
/*
 * Abstract class 'BedType' implements Bed's virtual container of features/reads.
 * It's not implemented in Bed because of functionality differentiation.
 * Strongly needs keyword 'abstract', but it doesn't compiled by GNU g++
 */
{
protected:
	typedef typename vector<I>::iterator ItemsIter;

public:
	typedef typename vector<I>::const_iterator cItemsIter;

private:
	// Initializes size of positions container.
	inline void ReserveItemContainer(ULONG size) { _items.reserve(size); }

	// Checks the element for the new potential start/end positions for all possible ambiguous.
	//	@it: iterator reffering to the compared element
	//	@rgn: checked start/stop positions
	//	@ambig: possible ambiguities
	//  return: true if item should be accepted; otherwise false
	virtual bool CheckItemPos(ItemsIter it, const Region& rgn, Ambig& ambig) = 0;

	// Checks the last element for the new potential start/end positions for all possible ambiguous.
	//	@rgn: checked start/stop positions
	//	@ambig: possible ambiguities
	//  return: true if item should be accepted; otherwise false
	inline bool CheckLastPos(const Region& rgn, Ambig& ambig) {
		return CheckItemPos(_items.end()-1, rgn, ambig);
	}

	// Shrink size of positions container.
	inline void ShrinkItemContainer() {
	// actually shrink_to_fit() defined in C++11, 
	// but we cannot identify version of standart via __cplusplus
	// because in gcc++ __cplusplus often returned 1 in any case.
	// So for any case replace shrink_to_fit() by swap() for gcc++
#ifdef OS_Windows	
		_items.shrink_to_fit();
#else
		vector<I>(_items).swap(_items);
#endif
	}

	// Gets region by container iterator.
	virtual const Region Regn(cItemsIter it) const = 0;

	// Sorts and rechecks items for the ambiguities.
	//	@ambig:  ambiguous filters
	void SortItems(Ambig& ambig)
	{
		ULONG	rmvCnt = 0;		// counter of removed items in current chrom
		ItemsIter it, itLast;

		for(Iter cit=Begin(); cit!=End(); cit++) {
			// reduce indexes after previous chrom recheck
			it		= _items.begin() + (cit->second.FirstInd -= rmvCnt);
			itLast	= _items.begin() + (cit->second.LastInd  -= rmvCnt);
			_itemsCnt -= rmvCnt;
			// sort items for current chrom
			sort(it, itLast+1, I::CompareByStartPos);
			// recheck current chrom
			for(rmvCnt=0, it++; it<=itLast; it++)
				if( !CheckItemPos(it-1, Regn(it), ambig) ) {
					// ambiguous situation: remove current item
					_items.erase(it--);
					cit->second.LastInd--;
					itLast--;
					rmvCnt++;
				}
		}
	}

protected:
	vector<I> _items;	// vector of features/reads included in bed-file
	ULONG	_itemsCnt;	// count of all items. Initially it is equal to featrs.size(),
						// but may be reduced during extention

	//inline vector<I>* ItemsPoint() const { return &_items; };
	//inline BYTE	SizeOfItem() const { return sizeof(I); }

	// Returns a constan iterator referring to the first item of specified chrom
	//	@cit: chromosome's constant iterator
	inline const cItemsIter cItemsBegin(cIter cit) const {
		return _items.begin() + cit->second.FirstInd; 
	}

	// Returns a constant iterator referring to the past-the-end item of specified chrom
	//	@cit: chromosome'sconstant  iterator
	inline const cItemsIter cItemsEnd(cIter cit) const { 
		return _items.begin() + cit->second.LastInd + 1;
	}

	// Returns an iterator referring to the first item of specified chrom
	//	@cit: chromosome's constant iterator
	inline const ItemsIter ItemsBegin(cIter cit) {
		return _items.begin() + cit->second.FirstInd; 
	}

	// Returns an iterator referring to the past-the-end item of specified chrom
	//	@cit: chromosome's constant iterator
	inline const ItemsIter ItemsEnd(cIter cit) { 
		return _items.begin() + cit->second.LastInd + 1;
	}

	// Sets count of all features/reads.
	virtual void SetAllItemsCount()	{ _itemsCnt = _items.size(); }

	// Gets count of all features/reads.
	inline size_t AllItemsCount() const { return _itemsCnt; }

	// Gets count of items for chromosome or all by default.
	inline size_t ItemsCount(chrid cID) const {
		return cID==Chrom::UnID ? AllItemsCount() : At(cID).ItemsCount();
	}

	// Gets item.
	//	@it: chromosome's iterator
	//	@iInd: index of item
	inline const I& Item(cIter it, chrlen iInd) const {
		return _items[it->second.FirstInd + iInd];
	}

	// Returns item.
	//	@cInd: index of chromosome
	//	@iInd: index of item
	inline const I& Item(chrid cID, chrlen iInd) const { return Item(GetIter(cID), iInd); }

#ifdef DEBUG
	inline void PrintItem(chrlen itemInd) const { _items[itemInd].Print(); }
#endif

};

#if !defined _ISCHIP && !defined _WIGREG

class BedR : public BedType<Read>
/*
 * Class 'Bed Reads' represents bed-file as a list of chromosomes and container of Reads.
 */
{
private:
	readlen	_readLen;			// length of Read
	int		_minScore;			// score threshold: Reads with score <= _minScore are skipping
	readscr	_maxScore;			// maximum score along Reads
#ifdef _BEDR_EXT
	Read::rNameType	_rNameType;		// type of Read name
	bool		_paired;		// true if Reads are paired-end
#endif
	// Gets a copy of region by container iterator.
	inline Region const Regn(cItemsIter it) const { return Region(it->Pos, it->Pos + _readLen); }

	// Checks the element for the new potential start/end positions for all possible ambiguous.
	//	@it: iterator reffering to the compared element
	//	@rgn: checked start/stop positions
	//	@ambig: possible ambiguities
	//  return: true if item should be accepted; otherwise false
	bool CheckItemPos(ItemsIter it, const Region& rgn, Ambig& ambig);

	// Adds Read to the container.
	//	@rgn: Region with mandatory fields
	//	@file: file to access to additionally fields
	//	return: true if Read was added successfully
	bool AddPos(const Region& rgn, TabFile& file);

	// Decreases Read's start position without checkup indexes.
	//	@cID: chromosome's ID
	//	@rInd: index of read
	//	@shift: decrease read's start position on this value
	//	@rgEnd: region's end position to control Read location
	//	@return: true if Read is insinde this region
	//bool DecreasePos(chrid cID, chrlen rInd, chrlen shift, chrlen rgEnd);

public:
	// Creates new instance from bed-file with output info.
	// Invalid instance will be completed by throwing exception.
	//	@title: title printed before file name or NULL
	//	@fName: name of bed-file
	//	@cSizes: chrom sizes to control the chrom length exceedeng; if NULL, no control
	//	@info: type of feature ambiguties that should be printed
	//	@absolPrintfName: true if file name should be printed absolutely, otherwise deneds on info
	//	@abortInvalid: true if invalid instance should abort excecution
	//	@alarm: true if warning messages should be printed 
	//	@acceptDupl: true if duplicates are acceptable 
	//	@minScore: score threshold (Reads with score <= minScore are skipping)
	BedR(const char* title, const string& fName, const ChromSizes* cSizes, eInfo info,
		bool absolPrintfName, bool abortInvalid, bool alarm, bool acceptDupl=true, int minScore=vUNDEF )
		: _readLen(0), _minScore(minScore)
#ifdef _BEDR_EXT
		, _rNameType(Read::nmUndef), _paired(false), _maxScore(0)
#endif
	{ 
		Ambig ambig(info, alarm, FT::ABED,
			acceptDupl ? Ambig::ACCEPT : Ambig::OMIT_SILENT,	// duplicated reads
			Ambig::ACCEPT,		// crossed & adjacent reads: typical
			//ignoreDiffSize ? Ambig::OMIT : Ambig::ABORTING
			Ambig::OMIT			// different Read size
		);
		Init(title, fName, ambig, const_cast<ChromSizes*>(cSizes), info > iLAC || absolPrintfName, abortInvalid);
	}

#ifdef _BEDR_EXT
	// Gets maximum score
	inline Read::rNameType ReadNameType()	const { return _rNameType; }
	//inline bool ReadPositionName ()	const { return _rNameType; }

	// Gets maximum score
	inline readscr MaxScore()		const { return _maxScore; }
#endif
	// Gets an item's title
	//	@pl: true if plural form
	inline const string& ItemTitle(bool pl = false) const { return FT::ItemTitle(FT::ABED, pl); };

	// Gets length of Read.
	inline readlen ReadLen()		const { return _readLen; }

	// Returns Read's start position.
	//	@cID: chromosome's ID
	//	@rInd: index of read
	inline chrlen ReadPos(chrid cID, chrlen rInd=0) const { return Item(cID, rInd).Pos;	}

	// Returns iterator to first Read.
	//	@cit: chrom's const iterator
	inline cItemsIter ReadsBegin(cIter cit) const { return cItemsBegin(cit); }

	// Returns iterator to first Read.
	//	@cID: chrom's ID
	inline cItemsIter ReadsBegin(chrid cID) const { return ReadsBegin(GetIter(cID)); }

	// Returns iterator to last Read.
	//	@cit: chrom's const iterator
	inline cItemsIter ReadsEnd(cIter cit)	const { return cItemsEnd(cit); }

	// Gets count of Reads for chromosome or all by default.
	inline size_t ReadsCount(chrid cID=Chrom::UnID) const	{ return ItemsCount(cID); }
};
#endif	// !_ISCHIP && !_WIGREG

#ifndef _VALIGN
#ifdef _ISCHIP
struct Featr : public Region
{
	//const string	Name;	// features's name
	readscr	Score;		// features's score

	inline Featr(const Region& rgn, readscr score=1) : Region(rgn), Score(score) {}

#ifdef DEBUG
	void Print() const {
		cout << Start << TAB << End << TAB;
		//if( Name )	cout << Name << TAB;
		cout << Score << EOL;
	}
#endif	// DEBUG
};
#else	// NO _ISCHIP
typedef Region	Featr;
#endif	// _ISCHIP

class BedF : public BedType<Featr>
/*
 * Class 'Bed Features' represents bed-file as a list of chromosomes and container of features.
 * Joines adjacent features, which are commonly encountered in bedgraph.
 */
{
private:
	static const BYTE _MinFieldsCnt = 3;	// minimum count of fields readed from file

#ifdef _ISCHIP
	readlen	_minFtrLen;		// minimal length of feature
	float	_maxScore;		// maximal feature score after reading
#elif defined _BIOCC
	// these vars needed to get warning if user call BedR instead of BedF (without -a option)
	long	_fLen;		// feature's length. 'long' since we compare the difference with Read length
	bool	_unifLen;	// true if all features have the same length
#endif	// _ISCHIP

	// Sets new end position on the feature if necessary.
	//	@it: iterator reffering to the feature which end position may be corrected
	//	@end: potential new end position
	//	@treatCaseRes: result of treatment this ambiguity
	//	return: true if ambiguity is permitted (feature is valid)
	bool CorrectItemsEnd(ItemsIter it, chrlen end, int treatCaseRes);

	// Gets item's title
	//	@pl: true if plural form
	inline const string& ItemTitle(bool pl=false) const	{ return FT::ItemTitle(FT::BED, pl); }
	
	// Gets a copy of Region by container iterator.
	inline Region const Regn(cItemsIter it) const { return *it; }

	// Checks the element for the new potential start/end positions for all possible ambiguous.
	//	@it: iterator reffering to the compared element
	//	@rgn: checked start/stop positions
	//	@ambig: possible ambiguities
	//  return: true if item should be accepted; otherwise false
	bool CheckItemPos(ItemsIter it, const Region& rgn, Ambig& ambig);

	// Adds feature to the container
	//	@rgn: Region with mandatory fields
	//	@file: file file to access to additionally fields
	//	return: true if Read was added successfully
	bool AddPos(const Region& rgn, TabFile& file);

#ifdef _ISCHIP
	// Scales defined score through all features to the part of 1.
	void ScaleScores ();
#endif
	// Decreases Feature's positions without checkup indexes.
	//	@cID: chromosome's ID
	//	@fInd: index of Feature
	//	@shift: decrease Feature's positions on this value
	//	@rgEnd: region's end position to control feature location
	//	@return: true if Feature is insinde this region
	//bool DecreasePos(chrid cID, chrlen fInd, chrlen shift, chrlen rgEnd);

public:
#ifdef _ISCHIP
	// Creates new instance by bed-file name
	// Invalid instance wil be completed by throwing exception.
	//	@title: title printed before file name or NULL
	//	@fName: file name
	//	@cSizes: chrom sizes to control the chrom length exceedeng; if NULL, no control
	//	@info: type of feature ambiguties that should be printed
	//	@absolPrintfName: true if file name should be printed absolutely, otherwise deneds on info
	//	@bsLen: length of binding site: shorter features would be omitted
	//	@alarm: true if info about ambiguous lines is printed during initialization
	BedF(const char* title, const string& fName, const ChromSizes* cSizes, eInfo info,
		bool absolPrintfName, readlen bsLen, bool alarm)
		: _minFtrLen(bsLen), _maxScore(vUNDEF)
	{
		Ambig ambig(info, alarm, FT::BED);
		Init(title, fName, ambig, const_cast<ChromSizes*>(cSizes), info > iLAC || absolPrintfName, true);
		ScaleScores();
	}
#else
	// Creates new instance by bed-file name
	// Invalid instance will be completed by throwing exception.
	//	@title: title printed before file name or NULL
	//	@fName: name of bed-file
	//	@cSizes: chrom sizes to control the chrom length exceedeng; if NULL, no control
	//	@info: type of feature ambiguties that should be printed
	//	@absolPrintfName: true if file name should be printed absolutely, otherwise deneds on info
	//	@abortInvalid: true if invalid instance should abort excecution
	//	@alarm: true if warning messages should be printed 
	BedF(const char* title, const string& fName, const ChromSizes* cSizes, eInfo info,
		bool absolPrintfName, bool abortInvalid, bool alarm)
#ifdef _BIOCC
		: _fLen(0), _unifLen(true)
#endif
	{
		Ambig ambig(info, alarm, FT::BED);
		Init(title, fName, ambig, const_cast<ChromSizes*>(cSizes), info > iLAC || absolPrintfName, abortInvalid);
	}
#endif	//  _ISCHIP
	
	// Gets chromosome's feature by ID
	//	@cID: chromosome's ID
	//	@fInd: feature's index, or first feature by default
	//inline const Featr& Feature(chrid cID, chrlen fInd=0) const { return Item(cID, fInd); }

	// Gets chromosome's feature by iterator
	//	@it: chromosome's iterator
	//	@fInd: feature's index, or first feature by default
	inline const Featr& Feature(cIter it, chrlen fInd=0) const { return Item(it, fInd); }

	// Gets count of features for chromosome or all by default.
	inline size_t FeaturesCount(chrid cID = Chrom::UnID) const	{ return ItemsCount(cID); }
	
	// Returns count of chromosome's features by its iter
	//	@it: chromosome's iterator
	chrlen FeaturesCount(cIter it) const { return it->second.ItemsCount(); }

	// Gets chromosome's treated length:
	// a double length for numeric chromosomes or a single for named.
	//	@it: chromosome's iterator
	//	@multiplier: 1 for numerics, 0 for letters
	//	@fLen: average fragment length on which each feature will be expanded in puprose of calculation
	//	(float to minimize rounding error)
	ULONG FeaturesTreatLength(cIter it, BYTE multiplier, float fLen) const;

	// Gets chromosome's treated length:
	// a double length for numeric chromosomes or a single for named.
	//	@cID: chromosome's ID
	//	@multiplier: 1 for numerics, 0 for nameds
	//	@fLen: average fragment length on which each feature will be expanded in puprose of calculation
	//	(float to minimize rounding error)
	inline ULONG FeaturesTreatLength(chrid cID, BYTE multiplier, float fLen) const {
		return FeaturesTreatLength(GetIter(cID), multiplier, fLen);
	}

	// Expands all features positions on the fixed length in both directions.
	// If extended feature starts from negative, or ends after chrom length, it is fitted.
	//	@extLen: distance on which Start should be decreased, End should be increased,
	//	or inside out if it os negative
	//	@cSizes: chrom sizes
	//	@info: displayed info
	//	return: true if positions have been changed
	bool Extend(int extLen, const ChromSizes* cSizes, eInfo info);

	// Checks whether all features length exceed gien length, throws exception otherwise.
	//	@len: given control length
	//	@lenDefinition: control length definition to print in exception message
	//	@sender: exception sender to print in exception message
	void CheckFeaturesLength(chrlen len, const string lenDefinition, const char* sender);

#ifndef _ISCHIP

	// Returns iterator to first feature.
	//	@cit: chrom's const iterator
	inline cItemsIter FeaturesBegin(cIter cit) const { return cItemsBegin(cit); }

	// Returns iterator to last feature.
	//	@cit: chrom's const iterator
	inline cItemsIter FeaturesEnd(cIter cit) const	{ return cItemsEnd(cit); }

	// Gets the ordinary total length of all chromosome's features
	//	@it: chromosome's iterator
	inline chrlen FeaturesLength(cIter it) const {
		return chrlen(FeaturesTreatLength(it, 0, 0));
	}

	// Gets the ordinary total length of all chromosome's features
	//	@cID: chromosome's ID
	//inline chrlen FeaturesLength(chrid cID) const {
	//	return chrlen(FeaturesTreatLength(cID, 0, 0));
	//}

	// Copies features coordinates to external Regions.
	void FillRegions(chrid cID, Regions& regn) const;

#endif	// _ISCHIP

#ifdef _BIOCC
	// Returns true if all features have the same length
	inline bool	SameFeaturesLength() const { 
		return _unifLen; 
	}

	friend class JointedBeds;	// to access GetIter(chrid)
#endif
};
#endif

// 'Nts' represented chromosome as array of nucleotides from fa-file
class Nts
{
private:
	char*	_nts;			// the nucleotides buffer
	chrlen	_len;			// total length of chromosome
	chrlen	_cntN;			// the number of 'N' nucleotides
	Regions	_defRgns;		// defined regions
	Region	_commonDefRgn;	// common defined region (except N at the begining and at the end)

	// Copy current readed line to the nucleotides buffer.
	//	@line: current readed line
	//	@lineLen: current readed line length
	void CopyLine(const char* line, chrlen lineLen);

	// Creates a new 'full' instance
	//	@fName: file name
	//	@minGapLen: minimal length which defines gap as a real gap
	//	@fillNts: if true fill nucleotides and def regions, otherwise def regions only
	//	@letN: if true then include 'N' on the beginning and on the end 
	//	Exception: Err.
	void	Init(const string& fName, short minGapLen, bool fillNts, bool letN);

public:
	
	// Creates a new empty instance (without nucleotides)
	//	@fName: FA file name
	//	Exception: Err
	inline Nts (const string& fName)	{ Init(fName, 0, false, true); }

	// Creates a new rich instance (with nucleotides)
	//	@fName: FA file name
	//	@letN: if true then include 'N' on the beginning and on the end 
	//	Exception: Err
	inline Nts (const string& fName, bool letN)	{ Init(fName, 0, true, letN); }

	// Creates a new empty instance (without nucleotides) with filling regions
	//	@fName: FA file name
	//	@minGapLen: minimal length which defines gap as a real gap
	//	@letN: if true then include 'N' on the beginning and on the end 
	//	Exception: Err
	inline Nts (const string& fName, short minGapLen, bool letN)
	{ Init(fName, minGapLen, false, letN); }

	inline ~Nts()	{ if(_nts) { delete [] _nts; _nts = NULL; } }

	// Gets Read on position or NULL if the rest is shorter than Read length
	const char* Read(const chrlen pos) const { 
		return (pos + Read::Len) < _len ? _nts + pos : NULL;
	}

	// Gets full count of nucleotides
	inline chrlen Length()	const { return _len; }

#ifdef _ISCHIP

	// Gets count of nucleotides within defined region
	inline chrlen DefLength() const { return _len - _commonDefRgn.Length(); }

	// Gets feature with defined region (without N at the begining and at the end) and score of 1
	inline const Featr DefRegion() const { return Featr(_commonDefRgn); }
	
	// Gets start position of first defined nucleotide
	inline chrlen Start()	const { return _commonDefRgn.Start; }
	
	// Gets end position of last defined nucleotide
	//inline chrlen End()		const { return _commonDefRgn.End; }
	
	// Gets total number of 'N' nucleotides
	inline chrlen CountN()	const { return _cntN; }

#else	// _ISCHIP

	// Gets defined nucleotides regions
	inline const Regions& DefRegions() const { return _defRgns; }

#endif	// _ISCHIP

#if defined _FILE_WRITE && defined DEBUG 
	// Saves instance to file by fname
	void Write(const string & fname, const char *chrName) const;
#endif
};

// 'ChrFileLen' represented chromosome's file attributes for class 'ChromFiles'
struct ChrFileLen
{
private:
	chrlen	_fileLen;	// length of uncompressed file or 0 if chrom is not treated
#ifdef _ISCHIP
	BYTE	_numeric;	// 1 for numeric chomosomes, 0 for named; used as bit shift
#endif

	inline ChrFileLen(const string& cName) : _fileLen(0)
#ifdef _ISCHIP
		, _numeric(isdigit(cName[0]) ? 1 : 0)	// isdigit() returns 0 or some integer
#endif
	{}

public:
	inline ChrFileLen() : _fileLen(0)
#ifdef _ISCHIP
		, _numeric(1)
#endif
	{}

	// true if this chromosome should be treated
	inline bool Treated() const { return _fileLen > 0; }

#ifdef _ISCHIP
	// Returns 1 if chrom name is numeric, otherwise 0
	inline BYTE Numeric() const { return _numeric; }

private:
	// Gets chromosome's treated length: a double length for numeric chromosomes, a single for named.
	//	@sizeFactor: ratio lenth_of_nts / size_of_file
	inline chrlen TreatLength(float sizeFactor) const
	{ return chrlen(_fileLen * sizeFactor) << _numeric; }
#endif

	friend class ChromFiles;	// to acces to private members
};

// 'ChromFiles' represented list of chromosomes with their file's attributes
class ChromFiles : public Chroms<ChrFileLen>
{
private:
	string	_path;			// files path
	string	_prefixName;	// common prefix of file names
	string	_ext;			// files extention
	bool	_extractAll;	// true if all chromosomes should be extracted. Used in imitator only

	// Returns length of common prefix before abbr chrom name of all file names
	//	@fName: full file name
	//	@extLen: length of file name's extention
	//	return: length of common prefix or -1 if there is no abbreviation chrom name in fName
	static int	CommonPrefixLength(const string& fName, BYTE extLen);

	// Fills external vector by chrom IDs relevant to file's names found in given directory.
	//	@files: empty external vector of file's names
	//	@gName: name of .fa files directory or single .fa file
	//	@cID: chromosomes ID that sould be treated, or Chrom::UnID if all
	//	return: count of filled chrom IDs
	//	Method first searches chroms among .fa files.
	//	If there are not .fa files or there are not .fa file for given cID,
	//	then searches among .fa.gz files
	BYTE GetChromIDs(vector<string>& files, const string & gName, chrid cID);

	// Adds chrom by name
	//	@cName: short chrom name
	inline void	AddChrom(const string& cName)	{ AddVal(Chrom::ID(cName), cName); }

public:
	// Creates and initializes an instance.
	//	@gName: name of .fa files directory or single .fa file. If single file, then set Chrom::StatedID()
	//	@treatAll: true if all chromosomes sould be extracted
	ChromFiles(const string& gName, bool extractAll = true);

	// Returns full file name or first full file name by default
	//	@cID: chromosome's ID
	const string FileName(chrid cID=0) const;
	
	// Returns common files prefix - the full name without chromosome number
	inline const string	FullCommonName() const { return _path + _prefixName; }

	// Gets chrom ID from first file name
	inline chrid FirstChromID() const {	return CID(cBegin()); }

	// Returns directory contained chrom files.
	inline const string& Path()	const { return _path; }

#ifdef _ISCHIP

	// Gets uncompressed length of file
	inline chrlen FirstFileLength () const { return cBegin()->second._fileLen; }

	// Sets actually treated chromosomes indexes and sizes according bed.
	//	@bed: template bed. If NULL, set all chromosomes
	//	return: count of treated chromosomes
	chrid	SetTreated	(const Bed* const bed);
	
	// Gets count of treated chromosomes.
	chrid	TreatedCount()	const;
	
	// Returns true if chromosome by iterator should be treated
	inline bool	IsTreated(cIter it) const { return _extractAll || it->second.Treated(); }
	
	// Gets chromosome's treated length: a double length for numeric chromosomes, a single for named.
	//	@it: ChromFiles iterator
	//	@sizeFactor: ratio lenth_of_nts / size_of_file
	inline chrlen ChromTreatLength(cIter it, float sizeFactor) const
	{ return it->second.TreatLength(sizeFactor); }

	inline const ChrFileLen& operator[] (chrid cID) const { return At(cID);	}

#endif
#ifdef DEBUG
	void Print() const;
#endif
};

// 'ChromSizes' represents a storage of chromosomes sizes.
class ChromSizes : public Chroms<chrlen>
{
private:
	mutable genlen _gsize;		// size of whole genome
//#ifdef _BIOCC
	//mutable chrlen _minsize;	// minimal size of chromosome
//#endif	// _BIOCC

#ifndef _NO_UNODMAP
	// typedef and SizeCompare are needed for temporary sorting elements before saving to file
	typedef pair<chrid,chrlen> cSize;

	static bool SizeCompare (const cSize & ci1, const cSize & ci2) {
		return ci1.first < ci2.first;
	}
#endif
	// Initializes instance from file.
	//	@fName: name of file.sizes
	void Init (const string& fName);

	inline void AddValFromFile(chrid cID, const ChromFiles& cFiles) {
		AddVal(cID, Nts(cFiles.FileName(cID)).Length());
	}

	// Saves instance to file
	//	@fName: full file name
	void Write(const string fName);

public:
	static const string	Ext;

	// Creates instance from file.
	//	@fName: name of file.sizes
	inline ChromSizes (const string& fName)	{ Init(fName); }

	// Creates a new instance by chrom files
	//	@cFiles: chrom files
	//	@printReport: if true then print report about generation/addition size file to dout
	// Reads an existing chrom sizes file if it exists, otherwise creates new instance.
	// Cheks and adds chrom if it is absent.
	// Saves instance to file if it is changed.
	ChromSizes (const ChromFiles& cFiles, bool printReport = true);

	// Gets chromosome's size by its ID.
	inline chrlen operator[] (chrid cID) const { return At(cID); }

	// Gets chromosome's size by its ID.
	inline chrlen Size (chrid cID) const { return At(cID); }

	// Gets total size of genome.
	genlen GenSize() const;
//#ifdef _BIOCC
	//chrlen MinSize() const;
//#endif	// _BIOCC
#ifdef DEBUG
	void Print() const;
#endif
};


#if defined _DENPRO || defined _BIOCC
// 'FileList' represents file's names incoming from argument list or from input list-file.
class FileList
/*
 * Under Windows should be translated with Character Set as not Unicode
 * (only 'Use Multi-Byte Character Set' or 'Not Set').
 */
{
private:
	char **_files;		// file names
	short _count;		// count of file names
	bool _memRelease;	// true if memory should be free in destructor

public:
	// Constructor for argument list.
	FileList	(char* files[], short cntFiles);
	// Constructor for list from input file.
	// Lines begining with '#" are commetns and would be skipped.
	FileList	(const char* fileName);
	~FileList();
	// Gets count of file's names.
	inline short Count() const { return _count; }
	inline char** Files() const { return _files; }
	inline const char* operator[](int i) const { return _files[i]; }
#ifdef DEBUG
	void Print() const;
#endif
};

// 'ChromRegions' represents chromosome's defined regions saved on file.
class ChromRegions : public Regions
{
private:
	static const string _FileExt;	// extention of files keeping chrom regions

public:
	// Creates an instance from file 'chrN.regions', if it exists.
	// Otherwise from .fa file then writes it to file 'chrN.regions'
	// File 'chrN.regions' is placed in @genomeName directory
	//	@commName: full common name of .fa files
	//	@cID: chromosome's ID.
	//	@minGapLen: minimal length which defines gap as a real gap
	ChromRegions(const string& commName, chrlen cID, short minGapLen);
};

// 'GenomeRegions' represents defined regions for each chromosome,
// initialized from ChromSizes (statically, at once)
// or from .fa files (dynamically, by request).
class GenomeRegions : public Chroms<Regions>
{
private:
	string _commonName;		// common part of chrom's file name (except chrom's number)
							// Used by initialization from ChromFiles only, to fill regions in run time
	const short	_minGapLen;	// minimal allowed length of gap
	const bool	_singleRgn;	// true if this instance has single Region for each chromosome

public:
	// Creates an instance by genome name, from chrom sizes file or genome.
	//	@gName: name of chrom sizes file, genome directory or single .fa file
	//	@cSizes: uninitialized pointer to chrom sizes; 
	//	after the constructor is completed, it is initialized with a new chrom sizes instance
	//	@minGapLen: minimal length which defines gap as a real gap
	GenomeRegions(const char* gName, const ChromSizes*& cSizes, short minGapLen);

	// Gets true if this instance has single Region for each chromosome
	// (is initialized from chrom.sizes file)
	inline bool	SingleRegions() const { return _singleRgn; }

	// Gets chrom's size by chrom's ID
	inline chrlen	Size(chrid cID)	const { return At(cID).LastEnd(); }

	// Gets chrom's size by chrom's iterator
	inline chrlen	Size(cIter it)	const { return it->second.LastEnd(); }

#ifdef _BIOCC
	// Gets miminal size of chromosome: for represented chromosomes only
	chrlen MinSize() const;

	// Gets total genome's size: for represented chromosomes only
	genlen GenSize() const;

	// Copying constructor: creates empty copy!
	inline GenomeRegions(GenomeRegions& gRgns) :
		_minGapLen(gRgns._minGapLen), _singleRgn(gRgns._singleRgn)
		{}

	// Adds chromosomes and regions without check up
	inline void AddChrom (chrid cID, const Regions& rgns) {	AddClass(cID, rgns); }
#endif	// _BIOCC

	// Returns Region for chromosome @cID.
	// If chromosome is not in collection, is's initialezed by _gDir
	// (in that case _gDir is a path)
	// In the last case it initializes from files 'chrN.regions', if are exist.
	// Otherwise these files are created and saved into genome directory
	inline const Regions & operator[] (chrid cID) {
		return FindChrom(cID) ? 
			At(cID) : AddClass(cID, ChromRegions(_commonName, cID, _minGapLen));
	}

#ifdef DEBUG
	void Print() const;
#endif
};
#endif	// _DENPRO || _BIOCC
