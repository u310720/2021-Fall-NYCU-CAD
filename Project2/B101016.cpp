#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <iomanip>
using namespace std;

struct Timing
{
    double **cell_rise, **cell_fall, **rise_transition, **fall_transition;
};
struct Lib
{
    vector<double> C_ZN_ref, Input_Transition_ref;
    double INV_C_I, INV_C_ZN, NAND_C_A1, NAND_C_A2, NAND_C_ZN, NOR_C_A1, NOR_C_A2, NOR_C_ZN;
    Timing INV, NAND, NOR;
};

enum Gate_type
{
    Pseudo,
    INV,
    NAND,
    NOR
};
enum Pin_num
{
    I,
    A1,
    A2,
};
struct Gate
{
    string ID, Output_Net_ID;
    int type, indegree, sensitization; // indegree is for topological sort, sensitization is for sensitive pins
    double C_ZN, Propagation_Delay, Transition_Time, Sensitive_Delay;
    bool ZN;
    Gate *I_ptr, *A1_ptr, *A2_ptr;    // ptr of from_gate, can get value by C_A1 == A1_ptr->C_ZN, A1 = A1_ptr->ZN, and so on
    vector<pair<int, Gate *>> ZN_ptr; // Pin_ToGate

    // pseudo gate
    explicit Gate(string net_id) : ID("pseudo_gate_" + net_id), Output_Net_ID(net_id), type(Pseudo), indegree(-1), sensitization(-1), C_ZN(0), Propagation_Delay(0), Transition_Time(0), Sensitive_Delay(0), ZN(0), I_ptr(nullptr), A1_ptr(nullptr), A2_ptr(nullptr)
    {
    }
    // normal gate
    explicit Gate(string _ID, int _type, Lib *lib) : ID(_ID), type(_type), sensitization(-1), Propagation_Delay(0), Transition_Time(0), Sensitive_Delay(0), ZN(0), I_ptr(nullptr), A1_ptr(nullptr), A2_ptr(nullptr)
    {
        ZN_ptr.clear();
        switch (_type)
        {
        case INV:
            indegree = 1;
            C_ZN = lib->INV_C_ZN;
            break;

        case NAND:
            indegree = 2;
            C_ZN = lib->NAND_C_ZN;
            break;
        case NOR:
            indegree = 2;
            C_ZN = lib->NOR_C_ZN;
            break;
        default:
            break;
        }
    }
};

struct Wire
{
    string FromGateID;
    vector<pair<int, Gate *>> Pin_ToGate;
};
struct Net
{
    map<string, Gate *> input;  // net_id, pseudo gate for applying input value
    map<string, Gate *> output; // net_id, from_gate
    map<string, Wire> wire;     // net_id, Wire
    map<string, Gate *> gate;   // gate's id, gate_ptr
    list<Gate *> topo_order;    // result of topological sorting of gates
};

void Extract_Value(string &s)
{
    bool flag = false;
    int len = s.length();
    for (int i = 0; i < len; i++)
    {
        if (s[i] == '_' && isdigit(s[i + 1]))
            s[i + 1] = ' ';
        switch (s[i])
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            break;
        default:
            s[i] = ' ';
            break;
        }
    }
}
double **Create_Table(int CZN_ref_size, int InTrans_ref_size)
{
    double **table = new double *[InTrans_ref_size];
    for (int i = 0; i < InTrans_ref_size; i++)
        table[i] = new double[CZN_ref_size];
    return table;
}
double **Load_Table(ifstream &ifs, int CZN_ref_size, int InTrans_ref_size)
{
    string line;
    double **table = Create_Table(CZN_ref_size, InTrans_ref_size);
    for (int i = 0; i < InTrans_ref_size; i++)
    {
        getline(ifs, line);
        Extract_Value(line);
        istringstream iss(line);
        string ss;
        for (int j = 0; j < CZN_ref_size; j++)
        {
            iss >> ss;
            table[i][j] = stod(ss);
        }
    }
    return table;
}
void Skip_Internal_Power(ifstream &ifs, string &line)
{
    int count = 0;
    while (count < 2)
    {
        getline(ifs, line);
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        count = line.find("}") != line.npos ? count + 1 : 0;
    }
}
Lib Load_Libaray(const string &_lib)
{
    Lib lib;
    ifstream ifs;
    ifs.open(_lib);
    if (!ifs)
    {
        cerr << "Lib could not open" << endl;
        return lib;
    }

    while (!ifs.eof())
    {
        int CZN_ref_size, InTrans_ref_size;
        string line;
        getline(ifs, line);
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        if (line.find("lu_table_template") != line.npos)
        {
            do
            {
                getline(ifs, line);
                if (line.find("index_1") != line.npos)
                {
                    Extract_Value(line);
                    string ss;
                    istringstream iss(line);
                    iss >> ss;
                    for (int i = 0; !iss.eof(); i++)
                    {
                        lib.C_ZN_ref.push_back(stod(ss));
                        iss >> ss;
                    }
                }
                else if (line.find("index_2") != line.npos)
                {
                    Extract_Value(line);
                    string ss;
                    istringstream iss(line);
                    iss >> ss;
                    for (int i = 0; !iss.eof(); i++)
                    {
                        lib.Input_Transition_ref.push_back(stod(ss));
                        iss >> ss;
                    }
                }
            } while (line.find("}") == line.npos);
            CZN_ref_size = lib.C_ZN_ref.size();
            InTrans_ref_size = lib.Input_Transition_ref.size();
        }
        else if (line.find("cell(NOR2X1){") != line.npos)
        {
            int count = 0;
            getline(ifs, line);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            do
            {
                if (line.find("pin(A1){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NOR_C_A1 = stod(line);
                        }
                    } while (line.find("}") == line.npos);
                }
                else if (line.find("pin(A2){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NOR_C_A2 = stod(line);
                        }
                    } while (line.find("}") == line.npos);
                }
                else if (line.find("pin(ZN){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NOR_C_ZN = stod(line);
                        }
                        else if (line.find("internal_power") != line.npos)
                            Skip_Internal_Power(ifs, line);
                        else if (line.find("cell_rise") != line.npos)
                            lib.NOR.cell_rise = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("cell_fall") != line.npos)
                            lib.NOR.cell_fall = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("rise_transition") != line.npos)
                            lib.NOR.rise_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("fall_transition") != line.npos)
                            lib.NOR.fall_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        count = line.find("}") != line.npos ? count + 1 : 0;
                    } while (count < 4);
                }
                getline(ifs, line);
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
            } while (count < 4);
        }
        else if (line.find("cell(INVX1){") != line.npos)
        {
            int count = 0;
            getline(ifs, line);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            do
            {
                if (line.find("pin(I){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.INV_C_I = stod(line);
                        }
                    } while (line.find("}") == line.npos);
                }
                else if (line.find("pin(ZN){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.INV_C_ZN = stod(line);
                        }
                        else if (line.find("internal_power") != line.npos)
                            Skip_Internal_Power(ifs, line);
                        else if (line.find("cell_rise") != line.npos)
                            lib.INV.cell_rise = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("cell_fall") != line.npos)
                            lib.INV.cell_fall = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("rise_transition") != line.npos)
                            lib.INV.rise_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("fall_transition") != line.npos)
                            lib.INV.fall_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        count = line.find("}") != line.npos ? count + 1 : 0;
                    } while (count < 4);
                }
                getline(ifs, line);
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
            } while (count < 4);
        }
        else if (line.find("cell(NANDX1){") != line.npos)
        {
            int count = 0;
            getline(ifs, line);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            do
            {
                if (line.find("pin(A1){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NAND_C_A1 = stod(line);
                        }
                    } while (line.find("}") == line.npos);
                }
                else if (line.find("pin(A2){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NAND_C_A2 = stod(line);
                        }
                    } while (line.find("}") == line.npos);
                }
                else if (line.find("pin(ZN){") != line.npos)
                {
                    do
                    {
                        getline(ifs, line);
                        line.erase(remove(line.begin(), line.end(), ' '), line.end());
                        if (line.find("capacitance") != line.npos)
                        {
                            Extract_Value(line);
                            lib.NAND_C_ZN = stod(line);
                        }
                        else if (line.find("internal_power") != line.npos)
                            Skip_Internal_Power(ifs, line);
                        else if (line.find("cell_rise") != line.npos)
                            lib.NAND.cell_rise = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("cell_fall") != line.npos)
                            lib.NAND.cell_fall = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("rise_transition") != line.npos)
                            lib.NAND.rise_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        else if (line.find("fall_transition") != line.npos)
                            lib.NAND.fall_transition = Load_Table(ifs, CZN_ref_size, InTrans_ref_size);
                        count = line.find("}") != line.npos ? count + 1 : 0;
                    } while (count < 4);
                }
                getline(ifs, line);
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
            } while (count < 4);
        }
    }
    ifs.close();
    return lib;
}

void Extract_Net(string &line, string title)
{
    line.erase(line.begin(), line.begin() + line.find(title) + title.length()); // erase title
    int len = line.length();
    for (auto &it : line)
        it = isdigit(it) || isalpha(it) ? it : ' ';
}
void Create_Input_Map(string &line, string title, map<string, Gate *> &i, map<string, Wire> &w)
{
    Extract_Net(line, title);
    istringstream iss(line);
    string ss;
    while (!iss.eof())
    {
        iss >> ss;
        Gate *pseudo_gate = new Gate(ss);
        Wire tmpW;
        i.insert(make_pair(ss, pseudo_gate));
        w.insert(make_pair(ss, tmpW));
    }
}
void Create_Output_Map(string &line, string title, map<string, Gate *> &o, map<string, Wire> &w)
{
    Extract_Net(line, title);
    istringstream iss(line);
    string ss;
    while (!iss.eof())
    {
        Wire tmp;
        iss >> ss;
        o.insert(make_pair(ss, nullptr));
        w.insert(make_pair(ss, tmp));
    }
}
void Create_Wire_Map(string &line, string title, map<string, Wire> &w)
{
    Extract_Net(line, title);
    istringstream iss(line);
    string ss;
    while (!iss.eof())
    {
        iss >> ss;
        Wire tmp;
        w.insert(make_pair(ss, tmp));
    }
}
void Set_Gate(string &line, int gate_type, Net &net, Lib *lib)
{
    istringstream iss;
    string ss, gate_id;
    Gate *gate;
    switch (gate_type)
    {
    case INV:
        Extract_Net(line, "INVX1");
        iss.str(line);
        iss >> gate_id; // gate_id
        gate = new Gate(gate_id, INV, lib);
        net.gate.insert(make_pair(gate_id, gate));
        while (!iss.eof())
        {
            iss >> ss;
            if (ss == "ZN")
            {
                iss >> ss; // net_id
                gate->Output_Net_ID = ss;
                auto itO = net.output.find(ss);
                if (itO != net.output.end())
                    itO->second = gate;
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.FromGateID = gate_id;
            }
            else if (ss == "I")
            {
                iss >> ss; // net_id
                auto itI = net.input.find(ss);
                if (itI != net.input.end())
                {
                    itI->second->ZN_ptr.push_back(make_pair(I, gate));
                    itI->second->C_ZN += lib->INV_C_I;
                }
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.Pin_ToGate.push_back(make_pair(I, gate));
            }
        }
        break;

    case NAND:
        Extract_Net(line, "NANDX1");
        iss.str(line);
        iss >> gate_id; // gate_id
        gate = new Gate(gate_id, NAND, lib);
        net.gate.insert(make_pair(gate_id, gate));
        while (!iss.eof())
        {
            iss >> ss;
            if (ss == "ZN")
            {
                iss >> ss; // net_id
                gate->Output_Net_ID = ss;
                auto itO = net.output.find(ss);
                if (itO != net.output.end())
                    itO->second = gate;
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.FromGateID = gate_id;
            }
            else if (ss == "A1")
            {
                iss >> ss; // net_id
                auto itI = net.input.find(ss);
                if (itI != net.input.end())
                {
                    itI->second->ZN_ptr.push_back(make_pair(A1, gate));
                    itI->second->C_ZN += lib->NAND_C_A1;
                }
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.Pin_ToGate.push_back(make_pair(A1, gate));
            }
            else if (ss == "A2")
            {
                iss >> ss; // net_id
                auto itI = net.input.find(ss);
                if (itI != net.input.end())
                {
                    itI->second->ZN_ptr.push_back(make_pair(A2, gate));
                    itI->second->C_ZN += lib->NAND_C_A2;
                }
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.Pin_ToGate.push_back(make_pair(A2, gate));
            }
        }
        break;

    case NOR:
        Extract_Net(line, "NOR2X1");
        iss.str(line);
        iss >> gate_id; // gate_id
        gate = new Gate(gate_id, NOR, lib);
        net.gate.insert(make_pair(gate_id, gate));
        while (!iss.eof())
        {
            iss >> ss;
            if (ss == "ZN")
            {
                iss >> ss; // net_id
                gate->Output_Net_ID = ss;
                auto itO = net.output.find(ss);
                if (itO != net.output.end())
                    itO->second = gate;
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.FromGateID = gate_id;
            }
            else if (ss == "A1")
            {
                iss >> ss; // net_id
                auto itI = net.input.find(ss);
                if (itI != net.input.end())
                {
                    itI->second->ZN_ptr.push_back(make_pair(A1, gate));
                    itI->second->C_ZN += lib->NOR_C_A1;
                }
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.Pin_ToGate.push_back(make_pair(A1, gate));
            }
            else if (ss == "A2")
            {
                iss >> ss; // net_id
                auto itI = net.input.find(ss);
                if (itI != net.input.end())
                {
                    itI->second->ZN_ptr.push_back(make_pair(A2, gate));
                    itI->second->C_ZN += lib->NOR_C_A2;
                }
                auto itW = net.wire.find(ss);
                if (itW != net.wire.end())
                    itW->second.Pin_ToGate.push_back(make_pair(A2, gate));
            }
        }
        break;
    default:
        break;
    }
}
void Set_Topo_Order(Net &net)
{
    vector<Gate *> stack;
    list<Gate *> gate_list;
    for (const auto &it : net.gate) // copy
        gate_list.push_back(it.second);
    for (auto &itI : net.input)
        for (auto &itZ : itI.second->ZN_ptr)
            itZ.second->indegree--;                           // input gate's indegree--
    for (auto it = gate_list.begin(); it != gate_list.end();) // initial stack
    {
        if ((*it)->indegree == 0)
        {
            stack.push_back(*it);
            gate_list.erase(it++);
        }
        else
            it++;
    }
    while (!stack.empty())
    {
        net.topo_order.push_back(stack.back());
        stack.pop_back();
        for (auto &it : net.topo_order.back()->ZN_ptr)
            if (--it.second->indegree == 0)
                stack.push_back(it.second);
    }
}
Net Load_Circuit(const string &_v, Lib *lib)
{
    Net net;
    ifstream ifs;
    ifs.open(_v);
    if (!ifs)
    {
        cerr << "Veriog file could not open." << endl;
        return net;
    }
    while (!ifs.eof())
    {
        string line;
        getline(ifs, line);
        if (line.find("input") != line.npos)
            Create_Input_Map(line, "input", net.input, net.wire);
        else if (line.find("output") != line.npos)
            Create_Output_Map(line, "output", net.output, net.wire);
        else if (line.find("//") == line.npos && line.find("wire") != line.npos)
            Create_Wire_Map(line, "wire", net.wire);
        else if (line.find("INVX1") != line.npos)
            Set_Gate(line, INV, net, lib);
        else if (line.find("NANDX1") != line.npos)
            Set_Gate(line, NAND, net, lib);
        else if (line.find("NOR2X1") != line.npos)
            Set_Gate(line, NOR, net, lib);
    }
    ifs.close();
    for (const auto &itI : net.input)
    {
        net.wire.find(itI.first)->second.FromGateID = itI.second->ID; // update FromGateID to pseudo gate's ID
        for (const auto &itZN : itI.second->ZN_ptr)                   // connect real input gate's pin to pseudo gate's ZN
        {
            switch (itZN.first)
            {
            case I:
                itZN.second->I_ptr = itI.second;
                break;

            case A1:
                itZN.second->A1_ptr = itI.second;
                break;

            case A2:
                itZN.second->A2_ptr = itI.second;
                break;

            default:
                break;
            }
        }
    }
    for (auto &itW : net.wire) // connect gate's
    {
        if (itW.second.FromGateID.find("pseudo") != string::npos) // if it's from pseudo gate, skip this round
            continue;
        Gate *from_gate = net.gate.find(itW.second.FromGateID)->second;
        for (auto &it : itW.second.Pin_ToGate)
        {
            switch (it.first)
            {
            case I:
                it.second->I_ptr = from_gate;
                from_gate->ZN_ptr.push_back(it);
                break;

            case A1:
                it.second->A1_ptr = from_gate;
                from_gate->ZN_ptr.push_back(it);
                break;

            case A2:
                it.second->A2_ptr = from_gate;
                from_gate->ZN_ptr.push_back(it);
                break;

            default:
                break;
            }
        }
    }
    for (auto &it : net.output)
        it.second->C_ZN = 0.03;
    Set_Topo_Order(net);
    return net;
}

void Output_Load(string &_case, Lib &lib, Net &net)
{
    ofstream ofs;
    string _load = "B101016_" + _case + "_load.txt";
    ofs.open(_load);
    if (!ofs)
    {
        cerr << _load << " could not open." << endl;
        return;
    }
    for (auto &itG : net.gate)
    {
        for (auto &itZN : itG.second->ZN_ptr)
        {
            switch (itZN.first)
            {
            case I:
                itG.second->C_ZN += lib.INV_C_I;
                break;
            case A1:
                if (itZN.second->type == NAND)
                    itG.second->C_ZN += lib.NAND_C_A1;
                else if (itZN.second->type == NOR)
                    itG.second->C_ZN += lib.NOR_C_A1;
                break;
            case A2:
                if (itZN.second->type == NAND)
                    itG.second->C_ZN += lib.NAND_C_A2;
                else if (itZN.second->type == NOR)
                    itG.second->C_ZN += lib.NOR_C_A2;
                break;
            default:
                break;
            }
        }
        ofs << itG.first << " " << itG.second->C_ZN << endl;
    }
    ofs.close();
}

void Set_Input(string &net_order, string &line, Net &net)
{
    bool value;
    string _net_id, _value;
    istringstream iss_order(net_order), iss_value(line);
    while (!iss_order.eof() && !iss_value.eof())
    {
        iss_order >> _net_id;
        iss_value >> _value;
        value = stoi(_value);
        net.input.find(_net_id)->second->ZN = value;
    }
}
bool Calc_ZN(Gate *G)
{
    if (G->type == INV)
        return !G->I_ptr->ZN;
    else if (G->type == NAND)
        return !(G->A1_ptr->ZN && G->A2_ptr->ZN);
    else if (G->type == NOR)
        return !(G->A1_ptr->ZN || G->A2_ptr->ZN);
    else
    {
        cerr << "Calc_ZN ERROR." << endl;
        return 0;
    }
}
int Find_Sensitization(Gate *G)
{
    if (G->type == INV)
    {
        G->Sensitive_Delay = G->I_ptr->Sensitive_Delay + G->I_ptr->Propagation_Delay;
        return I;
    }
    else if (G->type == NAND)
    {
        switch (G->A1_ptr->ZN * 2 + G->A2_ptr->ZN)
        {
        case 0B00:
            if (G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay > G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay)
            {
                G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
                return A2;
            }

            else
            {
                G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
                return A1;
            }
            break;

        case 0B01:
            G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
            return A1;
            break;

        case 0B10:
            G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
            return A2;
            break;

        case 0B11:
            if (G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay > G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay)
            {
                G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
                return A1;
            }
            else
            {
                G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
                return A2;
            }
            break;

        default:
            return EXIT_FAILURE;
            break;
        }
    }
    else if (G->type == NOR)
    {
        switch (G->A1_ptr->ZN * 2 + G->A2_ptr->ZN)
        {
        case 0B00:
            if (G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay > G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay)
            {
                G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
                return A1;
            }
            else
            {
                G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
                return A2;
            }
            break;

        case 0B01:
            G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
            return A2;
            break;

        case 0B10:
            G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
            return A1;
            break;

        case 0B11:
            if (G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay > G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay)
            {
                G->Sensitive_Delay = G->A2_ptr->Sensitive_Delay + G->A2_ptr->Propagation_Delay;
                return A2;
            }
            else
            {
                G->Sensitive_Delay = G->A1_ptr->Sensitive_Delay + G->A1_ptr->Propagation_Delay;
                return A1;
            }
            break;

        default:
            return EXIT_FAILURE;
            break;
        }
    }
    else
    {
        cerr << "Find_Sensitization ERROR." << endl;
        return EXIT_FAILURE;
    }
}
int Find_Interpolation_Index(vector<double> /*&*/ ref_list, double inter_num)
{
    // this sub will return the index of the first number greater than inter_num
    if (inter_num < ref_list[0]) // smaller than the minimum of ref_list
        return 1;                // use ref_list[0] and ref_list[1] as reference points
    int size = ref_list.size(), index = 0;
    for (; index < size; index++)
    {
        if (ref_list[index] > inter_num)
            return index;
    }
    return size - 1; // greater than the maximum of ref_list, use ref_list[size - 2] and ref_list[size - 1] as reference points
}
inline double Interpolation(double ref1, double value1, double ref2, double value2, double ans_ref)
{
    return (ans_ref - ref1) * (value2 - value1) / (ref2 - ref1) + value1;
}
double Interpolation_2D(double x1, double x2, double y1, double y2, double val_11, double val_12, double val_21, double val_22, double xInter, double yInter)
{
    double val_y1_XInter = Interpolation(x1, val_11, x2, val_12, xInter);
    double val_y2_XInter = Interpolation(x1, val_21, x2, val_22, xInter);
    return Interpolation(y1, val_y1_XInter, y2, val_y2_XInter, yInter);
}
void Calc_PropagationDelay_TransitionTime(Net &net, Lib &lib, Gate *G)
{
    int CZN_index, InputTrans_index;
    switch (G->type)
    {
    case INV:
        CZN_index = Find_Interpolation_Index(lib.C_ZN_ref, G->C_ZN);
        InputTrans_index = Find_Interpolation_Index(lib.Input_Transition_ref, G->I_ptr->Transition_Time);

        if (G->ZN) // high
        {
            G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.INV.cell_rise[InputTrans_index - 1][CZN_index - 1], lib.INV.cell_rise[InputTrans_index - 1][CZN_index], lib.INV.cell_rise[InputTrans_index][CZN_index - 1], lib.INV.cell_rise[InputTrans_index][CZN_index], G->C_ZN, G->I_ptr->Transition_Time);
            G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.INV.rise_transition[InputTrans_index - 1][CZN_index - 1], lib.INV.rise_transition[InputTrans_index - 1][CZN_index], lib.INV.rise_transition[InputTrans_index][CZN_index - 1], lib.INV.rise_transition[InputTrans_index][CZN_index], G->C_ZN, G->I_ptr->Transition_Time);
        }
        else // low
        {
            G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.INV.cell_fall[InputTrans_index - 1][CZN_index - 1], lib.INV.cell_fall[InputTrans_index - 1][CZN_index], lib.INV.cell_fall[InputTrans_index][CZN_index - 1], lib.INV.cell_fall[InputTrans_index][CZN_index], G->C_ZN, G->I_ptr->Transition_Time);
            G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.INV.fall_transition[InputTrans_index - 1][CZN_index - 1], lib.INV.fall_transition[InputTrans_index - 1][CZN_index], lib.INV.fall_transition[InputTrans_index][CZN_index - 1], lib.INV.fall_transition[InputTrans_index][CZN_index], G->C_ZN, G->I_ptr->Transition_Time);
        }
        break;

    case NAND:
        if (G->sensitization == A1)
        {
            CZN_index = Find_Interpolation_Index(lib.C_ZN_ref, G->C_ZN);
            InputTrans_index = Find_Interpolation_Index(lib.Input_Transition_ref, G->A1_ptr->Transition_Time);
            if (G->ZN) // high
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.cell_rise[InputTrans_index - 1][CZN_index - 1], lib.NAND.cell_rise[InputTrans_index - 1][CZN_index], lib.NAND.cell_rise[InputTrans_index][CZN_index - 1], lib.NAND.cell_rise[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.rise_transition[InputTrans_index - 1][CZN_index - 1], lib.NAND.rise_transition[InputTrans_index - 1][CZN_index], lib.NAND.rise_transition[InputTrans_index][CZN_index - 1], lib.NAND.rise_transition[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
            }
            else // low
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.cell_fall[InputTrans_index - 1][CZN_index - 1], lib.NAND.cell_fall[InputTrans_index - 1][CZN_index], lib.NAND.cell_fall[InputTrans_index][CZN_index - 1], lib.NAND.cell_fall[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.fall_transition[InputTrans_index - 1][CZN_index - 1], lib.NAND.fall_transition[InputTrans_index - 1][CZN_index], lib.NAND.fall_transition[InputTrans_index][CZN_index - 1], lib.NAND.fall_transition[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
            }
        }
        else if (G->sensitization == A2)
        {
            CZN_index = Find_Interpolation_Index(lib.C_ZN_ref, G->C_ZN);
            InputTrans_index = Find_Interpolation_Index(lib.Input_Transition_ref, G->A2_ptr->Transition_Time);
            if (G->ZN) // high
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.cell_rise[InputTrans_index - 1][CZN_index - 1], lib.NAND.cell_rise[InputTrans_index - 1][CZN_index], lib.NAND.cell_rise[InputTrans_index][CZN_index - 1], lib.NAND.cell_rise[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.rise_transition[InputTrans_index - 1][CZN_index - 1], lib.NAND.rise_transition[InputTrans_index - 1][CZN_index], lib.NAND.rise_transition[InputTrans_index][CZN_index - 1], lib.NAND.rise_transition[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
            }
            else // low
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.cell_fall[InputTrans_index - 1][CZN_index - 1], lib.NAND.cell_fall[InputTrans_index - 1][CZN_index], lib.NAND.cell_fall[InputTrans_index][CZN_index - 1], lib.NAND.cell_fall[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NAND.fall_transition[InputTrans_index - 1][CZN_index - 1], lib.NAND.fall_transition[InputTrans_index - 1][CZN_index], lib.NAND.fall_transition[InputTrans_index][CZN_index - 1], lib.NAND.fall_transition[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
            }
        }
        break;

    case NOR:
        if (G->sensitization == A1)
        {
            CZN_index = Find_Interpolation_Index(lib.C_ZN_ref, G->C_ZN);
            InputTrans_index = Find_Interpolation_Index(lib.Input_Transition_ref, G->A1_ptr->Transition_Time);
            if (G->ZN) // high
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.cell_rise[InputTrans_index - 1][CZN_index - 1], lib.NOR.cell_rise[InputTrans_index - 1][CZN_index], lib.NOR.cell_rise[InputTrans_index][CZN_index - 1], lib.NOR.cell_rise[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.rise_transition[InputTrans_index - 1][CZN_index - 1], lib.NOR.rise_transition[InputTrans_index - 1][CZN_index], lib.NOR.rise_transition[InputTrans_index][CZN_index - 1], lib.NOR.rise_transition[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
            }
            else // low
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.cell_fall[InputTrans_index - 1][CZN_index - 1], lib.NOR.cell_fall[InputTrans_index - 1][CZN_index], lib.NOR.cell_fall[InputTrans_index][CZN_index - 1], lib.NOR.cell_fall[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.fall_transition[InputTrans_index - 1][CZN_index - 1], lib.NOR.fall_transition[InputTrans_index - 1][CZN_index], lib.NOR.fall_transition[InputTrans_index][CZN_index - 1], lib.NOR.fall_transition[InputTrans_index][CZN_index], G->C_ZN, G->A1_ptr->Transition_Time);
            }
        }
        else if (G->sensitization == A2)
        {
            CZN_index = Find_Interpolation_Index(lib.C_ZN_ref, G->C_ZN);
            InputTrans_index = Find_Interpolation_Index(lib.Input_Transition_ref, G->A2_ptr->Transition_Time);
            if (G->ZN) // high
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.cell_rise[InputTrans_index - 1][CZN_index - 1], lib.NOR.cell_rise[InputTrans_index - 1][CZN_index], lib.NOR.cell_rise[InputTrans_index][CZN_index - 1], lib.NOR.cell_rise[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.rise_transition[InputTrans_index - 1][CZN_index - 1], lib.NOR.rise_transition[InputTrans_index - 1][CZN_index], lib.NOR.rise_transition[InputTrans_index][CZN_index - 1], lib.NOR.rise_transition[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
            }
            else // low
            {
                G->Propagation_Delay = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.cell_fall[InputTrans_index - 1][CZN_index - 1], lib.NOR.cell_fall[InputTrans_index - 1][CZN_index], lib.NOR.cell_fall[InputTrans_index][CZN_index - 1], lib.NOR.cell_fall[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
                G->Transition_Time = Interpolation_2D(lib.C_ZN_ref[CZN_index - 1], lib.C_ZN_ref[CZN_index], lib.Input_Transition_ref[InputTrans_index - 1], lib.Input_Transition_ref[InputTrans_index], lib.NOR.fall_transition[InputTrans_index - 1][CZN_index - 1], lib.NOR.fall_transition[InputTrans_index - 1][CZN_index], lib.NOR.fall_transition[InputTrans_index][CZN_index - 1], lib.NOR.fall_transition[InputTrans_index][CZN_index], G->C_ZN, G->A2_ptr->Transition_Time);
            }
        }
        break;

    default:
        cerr << "Calc_Propagation_Delay ERROR" << endl;
        break;
    }
}
void Find_Longest_Path(Net &net, list<string> &path, double &path_delay)
{
    Gate *path_gate = net.output.begin()->second;
    for (const auto &it : net.output) // find the output gate with the longest path from the output
        path_gate = path_gate->Sensitive_Delay + path_gate->Propagation_Delay > it.second->Sensitive_Delay + it.second->Propagation_Delay ? path_gate : it.second;
    path_delay = path_gate->Sensitive_Delay + path_gate->Propagation_Delay;

    while (path_gate->type != Pseudo)
    {
        path.push_front(path_gate->Output_Net_ID);
        switch (path_gate->sensitization)
        {
        case I:
            path_gate = path_gate->I_ptr;
            break;
        case A1:
            path_gate = path_gate->A1_ptr;
            break;
        case A2:
            path_gate = path_gate->A2_ptr;
            break;
        default:
            break;
        }
    }
    path.push_front(path_gate->Output_Net_ID);
}
void Output_Delay_And_Path(string &_case, Net &net, Lib &lib)
{
    ifstream ifs;
    string _pat = _case + ".pat";
    ofstream ofs_Delay, ofs_Path;
    string _delay = "B101016_" + _case + "_delay.txt", _path = "B101016_" + _case + "_path.txt";
    ifs.open(_pat);
    if (!ifs)
    {
        cerr << _pat << " could not open." << endl;
        return;
    }
    ofs_Delay.open(_delay);
    if (!ofs_Delay)
    {
        cerr << _delay << " could not open." << endl;
        return;
    }
    ofs_Path.open(_path);
    if (!ofs_Path)
    {
        cerr << _path << " could not open." << endl;
        return;
    }

    string net_order, line;
    getline(ifs, net_order);
    Extract_Net(net_order, "input");
    getline(ifs, line); // get first set of valus
    while (line.find(".end") == line.npos)
    {
        Set_Input(net_order, line, net);
        for (const auto &it : net.topo_order)
        {
            it->ZN = Calc_ZN(it);
            it->sensitization = Find_Sensitization(it);
            Calc_PropagationDelay_TransitionTime(net, lib, it);
        }
        getline(ifs, line);
        bool end_flag = (line.find(".end") != line.npos);

        // output delay.txt
        for (const auto &itG : net.gate)
        {
            ofs_Delay << itG.second->ID << " " << itG.second->ZN << " " << itG.second->Propagation_Delay << " " << itG.second->Transition_Time;
            if (itG != *net.gate.rbegin())
                ofs_Delay << endl;
        }
        if (!end_flag)
            ofs_Delay << endl
                      << endl;

        // output path.txt
        list<string> path;
        double path_delay;
        Find_Longest_Path(net, path, path_delay);
        ofs_Path << "Longest delay = " << path_delay << ", the path is: ";
        for (const auto &itP : path)
        {
            if (itP != path.back())
                ofs_Path << itP << " -> ";
            else
                ofs_Path << itP;
        }
        if (!end_flag)
            ofs_Path << endl;
    }
    ofs_Delay.close();
    ofs_Path.close();
}

int main(int argc, char **argv)
{
    string _case = "c17"; // default testing case name, for debug
    string _v, _pat, _lib;
    _v = argc == 6 ? argv[1] : _case + ".v";
    _pat = argc == 6 ? argv[3] : _case + ".pat";
    _lib = argc == 6 ? argv[5] : "test_lib.lib";
    _case.assign(_v.begin(), _v.end() - 2);

    Lib lib = Load_Libaray(_lib);
    Net net = Load_Circuit(_v, &lib);
    Output_Load(_case, lib, net);
    Output_Delay_And_Path(_case, net, lib);

    return 0;
}