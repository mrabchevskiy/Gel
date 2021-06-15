

#if !defined(CoreAGI__Gel_COCO_PARSER_H__)
#define CoreAGI__Gel_COCO_PARSER_H__



// #include "Scanner.h" // NB

namespace CoreAGI {
namespace Gel {


class Errors {
public:
	int count;			// number of errors detected

	Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_NAME=1,
		_QUOT=2,
		_TEXT=3,
		_ID=4,
		_INTEGER=5,
		_REAL=6,
		_NIHIL=7,
		_NOT=8,
		_EXCESS=9,
		_DOWNARROW=10,
		_RING=11
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

std::vector       < Term >         RPN;
  std::vector       < std::string  > SRC;
  std::unordered_set< std::wstring > VAR;
  std::vector       < std::wstring > ORD;

  unsigned must;      // :number of must be presented signs
  unsigned tabu;      // :number of tabu              signs
  bool     anonymous; // :anonymous entity

  void reset(){
    RPN.clear();
    RPN.push_back( Term() ); // :insert empty term
    SRC.clear();
    VAR.clear();
    ORD.clear();
    errors->count = 0;
  }

  std::string src( char separator ) const {
    std::stringstream S;
    bool first{ true };
    for( const auto& s: SRC ){
      if( not first ) S << separator;
      S << s;
      first = false;
    }
    return S.str();
  }

  void push( Term::Tag tag                 ){ RPN.push_back( Term( tag    ) ); }
  void push( Term::Tag tag, Token* t       ){ RPN.push_back( Term( tag, t ) ); }
  void push( Term::Tag tag, std::wstring s ){ RPN.push_back( Term( tag, s ) ); }

  Term pop(){ Term T = RPN.back(); RPN.pop_back(); return T; }

  

	Parser( Scanner* scanner );
	~Parser();
	void SemErr( const wchar_t* msg );

	void Gel();
	void General();
	void Modifier();
	void Query();
	void Select();
	void Analogy();
	void Forget();
	void Subject();
	void Entity();
	void Sequence();
	void Value();
	void Claim();
	void Lex();
	void Thing();
	void Sign();
	void Ref();

	void Parse();

}; // end Parser

} // namespace
} // namespace


#endif



#include <wchar.h>
// #include "Parser.h"  // NB
// #include "Scanner.h" // NB


namespace CoreAGI {
namespace Gel {


void Parser::SynErr( int n ){
	if( errDist >= minErrDist ) errors->SynErr( la->line, la->col, n );
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg ){
	if( errDist >= minErrDist ) errors->Error( t->line, t->col, msg );
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;                                                                  // :current token
		// if( dummyToken != t ) printf( "\n   `%s`", CONV( t->val ).c_str() );  // :NB DEBUG
		if( dummyToken != t ) SRC.push_back( CONV( t->val ).c_str() );           // :NB DEBUG
		la = scanner->Scan();
		if( la->kind <= maxT ){ ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::Gel() {
		switch (la->kind) {
		case _NAME: case _QUOT: case _ID: case 26 /* "(" */: {
			General();
			break;
		}
		case 19 /* ":" */: {
			Modifier();
			break;
		}
		case 12 /* "?" */: {
			Query();
			break;
		}
		case 21 /* "{" */: {
			Select();
			break;
		}
		case 25 /* "~" */: {
			Analogy();
			break;
		}
		case _NIHIL: {
			Forget();
			break;
		}
		default: SynErr(29); break;
		}
}

void Parser::General() {
		Subject();
		switch (la->kind) {
		case _EOF: {
			break;
		}
		case _RING: {
			Get();
			push( Term::E_ID ); push( Term::STOP );                                                    
			break;
		}
		case 12 /* "?" */: {
			Get();
			switch( RPN.size() ){
			 case 0 : SemErr( L"Internal compiler error (Empty RPN)" );                     break;
			 case 1 : SemErr( L"Missed subject" );                                          break;
			 case 2 : push( Term::SYND );                                                   break;
			 default: SemErr( L"Subject must be defined by name" );                         break;
			}
			push( Term::STOP );                                                                        
			break;
		}
		case _EXCESS: {
			Get();
			switch( RPN.size() ){
			 case 0 : SemErr( L"Internal compiler error (Empty RPN)" );                     break;
			 case 1 : SemErr( L"Missed subject" );                                          break;
			 case 2 : push( Term::DFNT );                                                   break;
			 default: RPN[0] = Term( Term::LIST ); push( Term::UNIQ ); push( Term::DFNT );  break;
			}
			push( Term::STOP );                                                                        
			break;
		}
		case 13 /* "`" */: {
			Get();
			Entity();
			Expect(12 /* "?" */);
			push( Term::GETA );                                                                        
			break;
		}
		case 14 /* "::=" */: {
			Get();
			switch( RPN.size() ){
			 case 0 : SemErr( L"Compiler error: Empty RPN" );                               break;
			 case 1 : SemErr( L"Missed subject" );                                          break;
			 case 2 :                                                                       break;
			 default: RPN[0] = Term( Term::LIST ); push( Term::UNIQ );                      break;
			}                                                                                          
			Entity();
			push( Term::BSRB ); push( Term::STOP );                                                    
			break;
		}
		case 15 /* "<=" */: {
			Get();
			switch( RPN.size() ){
			 case 0: SemErr( L"Compiler error: Empty RPN" ); break;
			 case 1: SemErr( L"Missed subject");             break;
			 case 2:                                         break;
			 default: push( Term::UNIQ );                    break;
			};                                                                                         
			if (la->kind == _NAME) {
				Get();
				push( Term::NAME, t ); push( Term::STOP );                                                 
			} else if (la->kind == _QUOT) {
				Get();
				push( Term::NAME, t ); push( Term::STOP );                                                 
			} else SynErr(30);
			break;
		}
		case 19 /* ":" */: {
			Modifier();
			if (la->kind == 17 /* "[" */) {
				Sequence();
			}
			Expect(16 /* "." */);
			if( RPN.size() < 2          ) SemErr( L"Empty statement" );
			if( must == 0 and anonymous ) SemErr( L"New anonymous entity with no signs" );
			if( tabu and anonymous      ) SemErr( L"Signs excl. for new anonymous entity" );
			push( Term::STOP );                                                                        
			break;
		}
		default: SynErr(31); break;
		}
}

void Parser::Modifier() {
		Expect(19 /* ":" */);
		anonymous = ( RPN.size() == 1 );
		switch( RPN.size() ){
		 case 0 : SemErr( L"Compiler error: Empty RPN" );   break;
		 case 1 : push( Term::ANON );                       break;
		 case 2 :                                           break;
		 default: push( Term::UNIQ );                       break;
		};
		tabu = must = 0;                                                                           
		while (StartOf(1)) {
			if (la->kind == _NOT) {
				Get();
				Entity();
				push( Term::EXCL ); tabu++;                                                                
			} else {
				Entity();
				push( Term::INCL ); must++;                                                                
				if (la->kind == 20 /* ":=" */) {
					Get();
					pop(); must--;                                                                             
					Value();
				}
			}
		}
}

void Parser::Query() {
		Expect(12 /* "?" */);
		if (StartOf(2)) {
			Entity();
			push( Term::EXPL );                                                                        
		} else if (la->kind == 12 /* "?" */) {
			Get();
			pop(); push( Term::INFO );                                                                 
		} else SynErr(32);
}

void Parser::Select() {
		Expect(21 /* "{" */);
		VAR.clear(); push( Term::CR_N );                                                           
		while (la->kind == _NAME) {
			Get();
			std::wstring W( t->val );
			if( VAR.contains( W ) ) SemErr( L"Duplicated variable" );
			VAR.insert( W ); ORD.push_back( W ); push( Term::SHOW );                                   
			if (la->kind == _DOWNARROW) {
				Get();
				pop(); push( Term::SKIP );                                                                 
			}
		}
		Expect(22 /* "|" */);
		if( VAR.empty()      ) SemErr( L"Missed variables" ); push( Term::COND );                  
		Claim();
		if( tabu + must == 0 ) SemErr( L"Empty Claim"      );                                      
		while (la->kind == 23 /* ";" */) {
			Get();
			Claim();
			if( tabu + must == 0 ) SemErr( L"Empty Claim"      );                                      
		}
		Expect(24 /* "}" */);
		push( Term::SLCT ); push( Term::STOP );                                                    
}

void Parser::Analogy() {
		Expect(25 /* "~" */);
		push( Term::CR_N );                                                                        
		if (StartOf(2)) {
			Entity();
			push( Term::SMLR );                                                                        
		} else if (la->kind == 21 /* "{" */) {
			Get();
			Thing();
			Thing();
			while (StartOf(2)) {
				Thing();
			}
			Expect(24 /* "}" */);
			push( Term::ANLG ); push( Term::STOP );                                                    
		} else SynErr(33);
}

void Parser::Forget() {
		Expect(_NIHIL);
		Entity();
		push( Term::KILL ); push( Term::STOP );                                                    
}

void Parser::Subject() {
		Entity();
		while (StartOf(2)) {
			Entity();
		}
}

void Parser::Entity() {
		if (la->kind == _NAME || la->kind == _QUOT || la->kind == _ID) {
			Lex();
		} else if (la->kind == 26 /* "(" */) {
			Ref();
		} else SynErr(34);
}

void Parser::Sequence() {
		Expect(17 /* "[" */);
		push( Term::NOPE );                                                                        
		while (StartOf(2)) {
			Entity();
		}
		Expect(18 /* "]" */);
		push( Term::SQNC );                                                                        
}

void Parser::Value() {
		if (la->kind == _REAL) {
			Get();
			push( Term::VALR, t ); push( Term::CR_Y );                                                 
		} else if (la->kind == _INTEGER) {
			Get();
			push( Term::VALI, t ); push( Term::CR_Y );                                                 
		} else if (la->kind == _TEXT) {
			Get();
			push( Term::VALS, t ); push( Term::CR_Y );                                                 
		} else if (la->kind == _NIHIL) {
			Get();
			push( Term::VALN, t ); push( Term::CR_Y );                                                 
		} else if (StartOf(2)) {
			Entity();
			Expect(13 /* "`" */);
			Entity();
			push( Term::VALA    ); push( Term::CR_Y );                                                 
		} else SynErr(35);
}

void Parser::Claim() {
		Lex();
		Expect(19 /* ":" */);
		tabu = must = 0;                                                                           
		while (StartOf(3)) {
			if (la->kind == _NOT) {
				Get();
				Lex();
				std::wstring W( t->val );
				if( VAR.contains( W ) ) SemErr( L"Variable can not be used in the `tabu` condition" );
				push( Term::DENY ); tabu++;                                                                
			} else {
				Lex();
				push( Term::INCL ); must++;                                                                
			}
		}
}

void Parser::Lex() {
		if (la->kind == _NAME) {
			Get();
			if( VAR.contains( t->val ) ) push( Term::VRBL, t ); else push( Term::CNST, t );            
		} else if (la->kind == _QUOT) {
			Get();
			push( Term::QUOT, t );                                                                     
		} else if (la->kind == _ID) {
			Get();
			push( Term::IDNT, t );                                                                     
		} else SynErr(36);
}

void Parser::Thing() {
		Entity();
		push( Term::SHOW );                                                                        
		if (la->kind == _DOWNARROW) {
			Get();
			pop(); push( Term::SKIP );                                                                 
		}
}

void Parser::Sign() {
		if (la->kind == _NOT) {
			Get();
			Entity();
			push( Term::TABU );                                                                        
		} else if (StartOf(2)) {
			Entity();
		} else SynErr(37);
}

void Parser::Ref() {
		Expect(26 /* "(" */);
		push( Term::LIST );                                                                        
		Sign();
		Sign();
		while (StartOf(1)) {
			Sign();
		}
		Expect(27 /* ")" */);
		push( Term::UNIQ );                                                                        
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};

	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};

	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	Gel();
	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 28;

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[4][30] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,T,T,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x},
		{x,T,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x},
		{x,T,T,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	ParserDestroyCaller<Parser>::CallDestroy(this);
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr( int line, int col, int n ){
	wchar_t* s;
	switch( n ){
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"NAME expected"); break;
			case 2: s = coco_string_create(L"QUOT expected"); break;
			case 3: s = coco_string_create(L"TEXT expected"); break;
			case 4: s = coco_string_create(L"ID expected"); break;
			case 5: s = coco_string_create(L"INTEGER expected"); break;
			case 6: s = coco_string_create(L"REAL expected"); break;
			case 7: s = coco_string_create(L"NIHIL expected"); break;
			case 8: s = coco_string_create(L"NOT expected"); break;
			case 9: s = coco_string_create(L"EXCESS expected"); break;
			case 10: s = coco_string_create(L"DOWNARROW expected"); break;
			case 11: s = coco_string_create(L"RING expected"); break;
			case 12: s = coco_string_create(L"\"?\" expected"); break;
			case 13: s = coco_string_create(L"\"`\" expected"); break;
			case 14: s = coco_string_create(L"\"::=\" expected"); break;
			case 15: s = coco_string_create(L"\"<=\" expected"); break;
			case 16: s = coco_string_create(L"\".\" expected"); break;
			case 17: s = coco_string_create(L"\"[\" expected"); break;
			case 18: s = coco_string_create(L"\"]\" expected"); break;
			case 19: s = coco_string_create(L"\":\" expected"); break;
			case 20: s = coco_string_create(L"\":=\" expected"); break;
			case 21: s = coco_string_create(L"\"{\" expected"); break;
			case 22: s = coco_string_create(L"\"|\" expected"); break;
			case 23: s = coco_string_create(L"\";\" expected"); break;
			case 24: s = coco_string_create(L"\"}\" expected"); break;
			case 25: s = coco_string_create(L"\"~\" expected"); break;
			case 26: s = coco_string_create(L"\"(\" expected"); break;
			case 27: s = coco_string_create(L"\")\" expected"); break;
			case 28: s = coco_string_create(L"??? expected"); break;
			case 29: s = coco_string_create(L"invalid Gel"); break;
			case 30: s = coco_string_create(L"invalid General"); break;
			case 31: s = coco_string_create(L"invalid General"); break;
			case 32: s = coco_string_create(L"invalid Query"); break;
			case 33: s = coco_string_create(L"invalid Analogy"); break;
			case 34: s = coco_string_create(L"invalid Entity"); break;
			case 35: s = coco_string_create(L"invalid Value"); break;
			case 36: s = coco_string_create(L"invalid Lex"); break;
			case 37: s = coco_string_create(L"invalid Sign"); break;

		default:
		{
			wchar_t format[ 20 ];
			coco_swprintf( format, 20, L"error %d", n );
			s = coco_string_create( format );
		}
		break;
	}
	//wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	char msg[ 256 ];
	if( line > 1 ) sprintf( msg, "E Line %d col %d: %s", line, col, CONV( s ).c_str() ); // NB
	else           sprintf( msg, "E Col %d: %s",               col, CONV( s ).c_str() ); // NB
	MSG.push_back( std::string( msg ) );                                                 // NB
	if( line == 1 ) LOC.insert( col-1 );                                                 // NB
	coco_string_delete( s );
	count++;
}

void Errors::Error( int line, int col, const wchar_t *s ){
	//wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	char msg[ 256 ];
	if( line > 1 ) sprintf( msg, "E Line %d col %d: %s", line, col, CONV( s ).c_str() ); // NB
	else           sprintf( msg, "E Col %d: %s",               col, CONV( s ).c_str() ); // NB
	MSG.push_back( std::string( msg ) );                                                 // NB
	if( line == 1 ) LOC.insert( col-1 );                                                 // NB
	count++;
}

void Errors::Warning( int line, int col, const wchar_t *s ){
	//wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	char msg[ 256 ];
	if( line > 1 ) sprintf( msg, "W Line %d col %d: %s", line, col, CONV( s ).c_str() ); // NB
	else           sprintf( msg, "W Col %d: %s",               col, CONV( s ).c_str() ); // NB
	MSG.push_back( std::string( msg ) );                                                 // NB
}

void Errors::Warning( const wchar_t *s ){
	//wprintf(L"%ls\n", s);
	char msg[ 256 ];                              // NB
	sprintf( msg, "W %s\n", CONV( s ).c_str() );  // NB
}

void Errors::Exception( const wchar_t* s ){
	//wprintf(L"%ls", s);
	printf( "\n\n Parser fatal error: %s", CONV( s ).c_str() );
	exit( EXIT_FAILURE );
}

} // namespace
} // namespace

