#include"backend/generator.h"
#include "backend/rv_def.h"
#include "backend/rv_inst_impl.h"
#include "front/semantic.h"
#include<assert.h>
#include<string>
#include<iostream>

#define todo() assert(0 && "todo")
#define warning_todo() std::cerr<<"This work is not complete in line "<<__LINE__<<"\n"
#define error() assert(0 && "RISCV generator error!")
#define dgen_global(name) this->fout<<("\t.global " + std::string(name) + " \n") //name必须是string 类型
#define dgen_type(name,type) this->fout<<("\t.type " + std::string(name) + ", @" + std::string(type) + " \n")
#define dgen_label(name) this->fout<<(std::string(name) + ":\n")
#define dgen_ins_str(ins_str) this->fout<<"\t"<<std::string(ins_str)<<" \n" 
#define literal_check(type_) (type_ == ir::Type::FloatLiteral || type_ == ir::Type::IntLiteral)
#define dgen_lw_ins(name, rs1_, rs2_, imm_) rv::rv_inst name; name.op = rv::rvOPCODE::LW; name.rs1 = rs1_; name.rs2 = rs2_; name.imm = imm_; rv_insts.push_back(name)
#define dgen_sw_ins(name, rs1_, rs2_, imm_) rv::rv_inst name; name.op = rv::rvOPCODE::SW; name.rs1 = rs1_; name.rs2 = rs2_; name.imm = imm_; stack_space+=4; rv_insts.push_back(name)
#define dgen_addi_ins(name, rd_, rs1_, imm_) rv::rv_inst name; name.op = rv::rvOPCODE::ADDI; name.rd = rd_; name.rs1 = rs1_; name.imm = imm_; rv_insts.push_back(name)
#define dgen_li_ins(name, rd_, imm_) rv::rv_inst name; name.op = rv::rvOPCODE::LI; name.rd = rd_; name.imm = imm_; rv_insts.push_back(name)
#define dgen_nop_ins(name) rv::rv_inst name; name.op = rv::rvOPCODE::NOP; rv_insts.push_back(name)
#define dgen_jr_ins(name, rs1_) rv::rv_inst name; name.op = rv::rvOPCODE::JR; name.rs1 = rs1_; rv_insts.push_back(name)


backend::Generator::Generator(ir::Program& p, std::ofstream& f): program(p), fout(f) {}

//实现draw函数
std::string rv::rv_inst::draw() const{
    std::string ret = "\t";
    switch (this->op)
    {
    //op
    case rv::rvOPCODE::NOP:
        ret += toString(this->op) + "\n";
        break;

    //op rd, imm
    case rv::rvOPCODE::LI:
        ret+= rv::toString(this->op) + "\t";
        ret+= rv::toString(this->rd) + ", ";
        ret+= std::to_string(this->imm) + "\n"; 
        break;
    //op rs1
    case rv::rvOPCODE::JR:
        ret+= rv::toString(this->op) + "\t";
        ret+= rv::toString(this->rs1) + "\n";
        break;
    // op rs2, imm(rs1)
    case rv::rvOPCODE::SW:
    case rv::rvOPCODE::LW:
        ret+= rv::toString(this->op) + "\t";
        ret+= rv::toString(this->rs2) + ", ";
        ret+= std::to_string(this->imm) + "("; 
        ret+= rv::toString(this->rs1) + ")\n";
        break;

    // op rd,rs1,imm
    case rv::rvOPCODE::ADDI:
        ret+= rv::toString(this->op) + "\t";
        ret+= rv::toString(this->rd) + ", ";
        ret+= rv::toString(this->rs1) + ", ";
        ret+= std::to_string(this->imm)+ "\n";
        break;
    default:
        error();
        break;
    }
    return ret;
}



void backend::Generator::gen() {
    /***声明全局变量****/
    //声明所有函数(除了global)
    for(ir::Function func:this->program.functions){
        if(func.name=="global"){
            continue;
        }
        dgen_global(func.name);
        dgen_type(func.name,"function");
    }
    //处理全局变量
    warning_todo();
    //处理函数
    for(ir::Function func:this->program.functions){
        if(func.name!="global"){
            gen_func(func);
        }
    }
}



void backend::Generator::gen_func(const ir::Function& func){
    //进入函数声明
    dgen_label(func.name);
    int stack_space = 0;
    std::vector<rv::rv_inst> rv_insts;
    /***函数开始工作**/
    //偏移栈指针
    //考虑数组过大....
    warning_todo();
    dgen_addi_ins(add_sp,rv::rvREG::sp,rv::rvREG::sp,0);//imm先占位,走完整个函数修改
    //存入帧指针
    dgen_sw_ins(sw_s0,rv::rvREG::sp,rv::rvREG::s0,0);//imm先占位,走完整个函数修改
    //偏移帧指针
    dgen_addi_ins(add_s0,rv::rvREG::s0,rv::rvREG::s0,0);//imm先占位,走完整个函数修改
    /*****进入函数指令*****/
    for(auto ins:func.InstVec){
        gen_instr(ins,rv_insts,stack_space);
    }
    //取出s0
    dgen_lw_ins(lw_s0,rv::rvREG::sp,rv::rvREG::s0,stack_space);
    //恢复sp
    dgen_addi_ins(recover_sp,rv::rvREG::sp,rv::rvREG::sp,stack_space);
    //填充占位
    rv_insts[0].imm = -stack_space;
    rv_insts[1].imm = stack_space;
    dgen_jr_ins(jr_ra,rv::rvREG::ra);
    //写入文件
    for(auto ri : rv_insts){
        fout<<ri.draw();
    }
}

void backend::Generator::gen_instr(ir::Instruction* ins, std::vector<rv::rv_inst>& rv_insts,int& stack_space){
    if(ins->op==ir::Operator::_return){
        if(literal_check(ins->op1.type)){
            if(ins->op1.type==ir::Type::IntLiteral){
                dgen_li_ins(li_literal,rv::rvREG::a0,this->analyzer.intstring_to_int(ins->op1.name));
            }
            else{
                todo();
            }
        }
        else{
            todo();
        }
    }
    else if(ins->op==ir::Operator::call){
        if(((ir::CallInst*)ins)->op1.name=="global"){
            //call global无效
            return;
        }
    }
    else{
        todo();
    }
}

std::string rv::toString(rvFREG r){
    todo();
}

std::string rv::toString(rvOPCODE r){
	if(r == rv::rvOPCODE::LW){return "lw";}
	else if(r == rv::rvOPCODE::SW){return "sw";}
	else if(r == rv::rvOPCODE::ADDI){return "addi";}
	else if(r == rv::rvOPCODE::LI){return "li";}
	else if(r == rv::rvOPCODE::NOP){return "nop";}
	else if(r == rv::rvOPCODE::JR){return "jr";}
	else{error();}
}



std::string rv::toString(rvREG r){
    if(r == rv::rvREG::X0){return "zero";}
    else if(r == rv::rvREG::X1){return "ra";}
    else if(r == rv::rvREG::X2){return "sp";}
    else if(r == rv::rvREG::X3){return "gp";}
    else if(r == rv::rvREG::X4){return "tp";}
    else if(r == rv::rvREG::X5){return "t0";}
    else if(r == rv::rvREG::X6){return "t1";}
    else if(r == rv::rvREG::X7){return "t2";}
    else if(r == rv::rvREG::X8){return "s0";}
    else if(r == rv::rvREG::X9){return "s1";}
    else if(r == rv::rvREG::X10){return "a0";}
    else if(r == rv::rvREG::X11){return "a1";}
    else if(r == rv::rvREG::X12){return "a2";}
    else if(r == rv::rvREG::X13){return "a3";}
    else if(r == rv::rvREG::X14){return "a4";}
    else if(r == rv::rvREG::X15){return "a5";}
    else if(r == rv::rvREG::X16){return "a6";}
    else if(r == rv::rvREG::X17){return "a7";}
    else if(r == rv::rvREG::X18){return "s2";}
    else if(r == rv::rvREG::X19){return "s3";}
    else if(r == rv::rvREG::X20){return "s4";}
    else if(r == rv::rvREG::X21){return "s5";}
    else if(r == rv::rvREG::X22){return "s6";}
    else if(r == rv::rvREG::X23){return "s7";}
    else if(r == rv::rvREG::X24){return "s8";}
    else if(r == rv::rvREG::X25){return "s9";}
    else if(r == rv::rvREG::X26){return "s10";}
    else if(r == rv::rvREG::X27){return "s11";}
    else if(r == rv::rvREG::X28){return "t3";}
    else if(r == rv::rvREG::X29){return "t4";}
    else if(r == rv::rvREG::X30){return "t5";}
    else if(r == rv::rvREG::X31){return "t6";}
    else if(r == rv::rvREG::null){return "null";}
    else{error();}
}




