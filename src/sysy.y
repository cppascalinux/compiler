%code requires {
	#include <memory>
	#include <string>
	#include "sysy.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "sysy.hpp"

using namespace std;
using namespace sysy;

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<CompUnit> &ast, const char *s);

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 , 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<sysy::CompUnit> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
	std::string *str_val;
	int int_val;
	sysy::Base *ast_val;
	void *void_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN ANDOP OROP CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP MULOP ADDOP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <void_val> BlockItemList ConstDefList VarDefList CompList FuncFParamList
%type <void_val> FuncRParamList DimList InitValList

%type <ast_val> FuncDef Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp
%type <ast_val> RelExp EqExp LAndExp LOrExp Decl ConstDecl ConstDef
%type <ast_val> BlockItem LVal InitVal VarDef VarDecl ClosedIf OpenIf NonIfStmt
%type <ast_val> CompItem FuncFParam


%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为  传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
	: CompList {
		auto comp_unit = make_unique<CompUnit>($1);
		ast = move(comp_unit);
	};

CompItem
	: FuncDef {
		auto ast = new FuncDefItem($1);
		$$ = ast;
	}
	| Decl {
		auto ast = new DeclItem($1);
		$$ = ast;
	};

CompList
	: CompItem {
		auto list = new vector<unique_ptr<CompItem> >();
		list->emplace_back(static_cast<CompItem*>($1));
		$$ = list;
	}
	| CompList CompItem {
		auto list = static_cast<vector<unique_ptr<CompItem> >*>($1);
		list->emplace_back(static_cast<CompItem*>($2));
		$$ = list;
	};

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了  之后
// 这种写法会省下很多内存管理的负担

FuncFParam
	: INT IDENT {
		auto ast = new FuncFParam(*unique_ptr<string>($2),
			new vector<unique_ptr<Exp> >());
		$$ = ast;
	}
	| INT IDENT '[' ']' DimList {
		auto list = static_cast<vector<unique_ptr<Exp> >*>($5);
		list->emplace(list->begin(), nullptr);
		auto ast = new FuncFParam(*unique_ptr<string>($2), list);
		$$ = ast;
	};

FuncFParamList
	: {
		$$ = new vector<unique_ptr<FuncFParam> >();
	}
	| FuncFParam {
		auto list = new vector<unique_ptr<FuncFParam> >();
		list->emplace_back(static_cast<FuncFParam*>($1));
		$$ = list;
	}
	| FuncFParamList ',' FuncFParam {
		auto list = static_cast<vector<unique_ptr<FuncFParam> >*>($1);
		list->emplace_back(static_cast<FuncFParam*>($3));
		$$ = list;
	};

FuncRParamList
	: {
		$$ = new vector<unique_ptr<Exp> >();
	}
	| Exp {
		auto list = new vector<unique_ptr<Exp> >();
		list->emplace_back(static_cast<Exp*>($1));
		$$ = list;
	}
	| FuncRParamList ',' Exp {
		auto list = static_cast<vector<unique_ptr<Exp> >*>($1);
		list->emplace_back(static_cast<Exp*>($3));
		$$ = list;
	};

FuncDef
	: INT IDENT '(' FuncFParamList ')' Block {
		auto ast = new FuncDef("int", *unique_ptr<string>($2), $4, $6);
		$$ = ast;
	}
	| VOID IDENT '(' FuncFParamList ')' Block {
		auto ast = new FuncDef("void", *unique_ptr<string>($2), $4, $6);
		$$ = ast;
	};

// 同上, 不再解释

Block
	: '{' BlockItemList '}' {
		auto ast = new Block($2);
		$$ = ast;
	};

Stmt
	: ClosedIf {
		$$ = $1;
	}
	| OpenIf {
		$$ = $1;
	};

ClosedIf
	: NonIfStmt {
		auto ast = new NonIfClosedIf($1);
		$$ = ast;
	}
	| IF '(' Exp ')' ClosedIf ELSE ClosedIf {
		auto ast = new IfElseClosedIf($3, $5, $7);
		$$ = ast;
	};

OpenIf
	: IF '(' Exp ')' Stmt {
		auto ast = new IfOpenIf($3, $5);
		$$ = ast;
	}
	| IF '(' Exp ')' ClosedIf ELSE OpenIf {
		auto ast = new IfElseOpenIf($3, $5, $7);
		$$ = ast;
	};

NonIfStmt
	: LVal '=' Exp ';' {
		auto ast = new AssignStmt($1, $3);
		$$ = ast;
	}
	| Exp ';' {
		auto ast = new ExpStmt($1);
		$$ = ast;
	}
	| ';' {
		auto ast = new ExpStmt(nullptr);
		$$ = ast;
	}
	| Block {
		auto ast = new BlockStmt($1);
		$$ = ast;
	}
	| RETURN Exp ';' {
		auto ast = new ReturnStmt($2);
		$$ = ast;
	}
	| RETURN ';' {
		auto ast = new ReturnStmt(nullptr);
		$$ = ast;
	}
	| WHILE '(' Exp ')' Stmt {
		auto ast = new WhileStmt($3, $5);
		$$ = ast;
	}
	| BREAK ';' {
		auto ast = new BreakStmt();
		$$ = ast;
	}
	| CONTINUE ';' {
		auto ast = new ContinueStmt();
		$$ = ast;
	}

Exp
	: LOrExp {
		auto ast = new Exp($1);
		$$ = ast;
	};

PrimaryExp
	: '(' Exp ')' {
		auto ast = new BracketExp($2);
		$$ = ast;
	}
	| LVal {
		auto ast = new LValExp($1);
		$$ = ast;
	}
	| INT_CONST {
		auto ast = new NumberExp($1);
		$$ = ast;
	};

UnaryExp
	: PrimaryExp {
		auto ast = new PrimUnaryExp($1);
		$$ = ast;
	}
	| ADDOP UnaryExp {
		auto ast = new OpUnaryExp(*unique_ptr<string>($1), $2);
		$$ = ast;
	}
	| '!' UnaryExp {
		auto ast = new OpUnaryExp("!", $2);
		$$ = ast;
	}
	| IDENT '(' FuncRParamList ')' {
		auto ast = new FuncUnaryExp(*unique_ptr<string>($1), $3);
		$$ = ast;
	};

MulExp
	: UnaryExp {
		auto ast = new MulExp(nullptr, "", $1);
		$$ = ast;
	}
	| MulExp MULOP UnaryExp {
		auto ast = new MulExp($1, *unique_ptr<string>($2), $3);
		$$ = ast;
	};

AddExp
	: MulExp {
		auto ast = new AddExp(nullptr, "", $1);
		$$ = ast;
	}
	| AddExp ADDOP MulExp {
		auto ast = new AddExp($1, *unique_ptr<string>($2), $3);
		$$ = ast;
	};

RelExp
	: AddExp {
		auto ast = new RelExp(nullptr, "", $1);
		$$ = ast;
	}
	| RelExp RELOP AddExp {
		auto ast = new RelExp($1, *unique_ptr<string>($2), $3);
		$$ = ast;
	};

EqExp
	: RelExp {
		auto ast = new EqExp(nullptr, "", $1);
		$$ = ast;
	}
	| EqExp EQOP RelExp {
		auto ast = new EqExp($1, *unique_ptr<string>($2), $3);
		$$ = ast;
	}

LAndExp
	: EqExp {
		auto ast = new LAndExp(nullptr, $1);
		$$ = ast;
	}
	| LAndExp ANDOP EqExp {
		auto ast = new LAndExp($1, $3);
		$$ = ast;
	};

LOrExp
	: LAndExp {
		auto ast = new LOrExp(nullptr, $1);
		$$ = ast;
	}
	| LOrExp OROP LAndExp {
		auto ast = new LOrExp($1, $3);
		$$ = ast;
	};

DimList
	: {
		auto list = new vector<unique_ptr<Exp> >();
		$$ = list;
	}
	| DimList '[' Exp ']' {
		auto list = static_cast<vector<unique_ptr<Exp> >*>($1);
		list->emplace_back(static_cast<Exp*>($3));
		$$ = list;
	};

LVal
	: IDENT DimList {
		auto ast = new LVal(*unique_ptr<string>($1), $2);
		$$ = ast;
	};

ConstDef
	: IDENT DimList '=' InitVal {
		auto ast = new ConstDef(*unique_ptr<string>($1), $2, $4);
		$$ = ast;
	};

ConstDefList
	: ConstDef {
		auto defs = new vector<unique_ptr<ConstDef> >();
		defs->emplace_back(static_cast<ConstDef*>($1));
		$$ =defs;
	}
	| ConstDefList ',' ConstDef {
		auto defs = static_cast<vector<unique_ptr<ConstDef> >*>($1);
		defs->emplace_back(static_cast<ConstDef*>($3));
		$$ =defs;
	};

ConstDecl
	: CONST INT ConstDefList ';' {
		auto ast = new ConstDecl($3);
		$$ = ast;
	};

Decl
	: ConstDecl {
		$$ = $1;
	}
	| VarDecl {
		$$ = $1;
	};

BlockItem
	: Decl {
		auto ast = new DeclBlockItem($1);
		$$ = ast;
	}
	| Stmt {
		auto ast = new StmtBlockItem($1);
		$$ = ast;
	};

BlockItemList
	: {
		auto items = new vector<unique_ptr<BlockItem> >();
		$$ = items;
	}
	| BlockItemList BlockItem {
		auto items = static_cast<vector<unique_ptr<BlockItem> >*>($1);
		items->emplace_back(static_cast<BlockItem*>($2));
		$$ = items;
	};

InitValList
	: {
		auto list = new vector<unique_ptr<InitVal> >();
		$$ = list;
	}
	| InitVal {
		auto list = new vector<unique_ptr<InitVal> >();
		list->emplace_back(static_cast<InitVal*>($1));
		$$ = list;
	}
	| InitValList ',' InitVal {
		auto list = static_cast<vector<unique_ptr<InitVal> >*>($1);
		list->emplace_back(static_cast<InitVal*>($3));
		$$ = list;
	};

InitVal
	: Exp {
		auto ast = new ExpInitVal($1);
		$$ = ast;
	}
	| '{' InitValList '}' {
		auto ast = new ListInitVal($2);
		$$ = ast;
	};

VarDef
	: IDENT DimList {
		auto ast = new VarDef(*unique_ptr<string>($1), $2, nullptr);
		$$ = ast;
	}
	| IDENT DimList '=' InitVal {
		auto ast = new VarDef(*unique_ptr<string>($1), $2, $4);
		$$ = ast;
	};

VarDefList
	: VarDef {
		auto defs = new vector<unique_ptr<VarDef> >();
		defs->emplace_back(static_cast<VarDef*>($1));
		$$ = defs;
	}
	| VarDefList ',' VarDef {
		auto defs = static_cast<vector<unique_ptr<VarDef> >*>($1);
		defs->emplace_back(static_cast<VarDef*>($3));
		$$ = defs;
	};

VarDecl
	: INT VarDefList ';'{
		auto ast = new VarDecl($2);
		$$ = ast;
	};

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<CompUnit> &ast, const char *s) {
	cerr << "error: " << s << endl;
}
