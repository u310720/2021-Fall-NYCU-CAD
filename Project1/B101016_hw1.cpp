#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

struct Term
{
    string value;
    vector<bool> PI_chart;
    bool mark;
};
void SetFileName(int argc, char **argv, string &inFile, string &outFile1, string &outFile2)
{
    if (argc == 1)
    {
        int FileNum = 1;
        inFile = "input" + to_string(FileNum) + ".txt";       // Default file name
        outFile1 = "implicant" + to_string(FileNum) + ".txt"; // Default file name
        outFile2 = "output" + to_string(FileNum) + ".txt";    // Default file name
    }
    else if (argc == 4)
    {
        inFile = argv[1];
        outFile1 = argv[2];
        outFile2 = argv[3];
    }
}
void ReadFile_and_Initial(string &inFile, int &Variables, map<int, int> &OnSet, vector<int> &DontCareSet)
{
    ifstream ifs(inFile, ios::in);
    if (!ifs)
    {
        cerr << inFile << " could not be opened!" << endl;
        exit(1);
    }
    string s;
    ifs >> s;
    if (s == ".i")
    {
        ifs >> s;
        Variables = stoi(s);
        ifs >> s;
    }
    if (s == ".m")
    {
        int OnSet_index = 0;
        while (!ifs.eof())
        {
            ifs >> s;
            if (s == ".d")
                break;
            else
                OnSet.insert(pair<int, int>(stoi(s), OnSet_index++));
        }
    }
    if (s == ".d")
    {
        while (!ifs.eof())
        {
            ifs >> s;
            DontCareSet.push_back(stoi(s));
        }
    }
}
int BitCount(string &v)
{
    int count = 0;
    for (const auto &it : v)
        if ((it == '1'))
            count++;
    return count;
}
string Dec2Bool(int n, int Variables)
{
    string v;
    while (n > 0)
    {
        if (n % 2 == 0)
            v.push_back('0');
        else
            v.push_back('1');
        n /= 2;
    }
    for (int i = v.size(); i < Variables; i++)
        v.push_back('0');
    reverse(v.begin(), v.end());
    return v;
}
void SetMinterms(int Variables, map<int, int> &OnSet, vector<int> &DontCareSet, vector<vector<Term>> &minterms)
{
    Term tmp;
    tmp.mark = false;

    int size = OnSet.size();
    for (const auto &itO : OnSet)
    {
        tmp.value = Dec2Bool(itO.first, Variables);
        tmp.PI_chart.clear();
        for (int i = 0; i < size; i++)
        {
            if (i == itO.second)
                tmp.PI_chart.push_back(true);
            else
                tmp.PI_chart.push_back(false);
        }
        minterms[BitCount(tmp.value)].push_back(tmp);
    }
    while (!DontCareSet.empty())
    {
        tmp.value = Dec2Bool(DontCareSet.back(), Variables);
        tmp.PI_chart.assign(size, 0);
        DontCareSet.pop_back();
        minterms[BitCount(tmp.value)].push_back(tmp);
    }
}
struct QM
{
    const int Variables;
    const map<int, int> &OnSet;
    vector<vector<Term>> &Minterms;
    vector<Term> &PrimeImplicant;

    bool CanMerge(Term &t1, Term &t2, Term &MergeResult)
    {
        int i = 0, count = 0;
        for (const auto &it : t1.value)
        {
            if (t1.value[i] != t2.value[i])
            {
                MergeResult.value.push_back('-');
                count++;
            }
            else
                MergeResult.value.push_back(it);
            if (count > 1)
                return false;
            i++;
        }
        for (i = 0; i < t1.PI_chart.size(); i++)
            MergeResult.PI_chart.push_back(t1.PI_chart[i] | t2.PI_chart[i]);
        MergeResult.mark = false;
        return true;
    }
    bool NotExistInGroup(vector<Term> &Group1, Term &MergeResult)
    {
        for (const auto &it : Group1)
            if (it.value == MergeResult.value)
                return false;
        return true;
    }
    void PutInPrimeImplicant(Term &t)
    {
        // Just put it in
        PrimeImplicant.push_back(t);
    }
    void PairInGroups(vector<Term> &Group1, vector<Term> &Group2)
    {
        int G1_Size = Group1.size();
        for (int i = 0; i < G1_Size; i++)
        {
            int j = 0;
            for (auto &it : Group2)
            {
                Term MergeResult;
                if (CanMerge(Group1[0], Group2[j], MergeResult))
                {
                    Group1[0].mark = true;
                    Group2[j].mark = true;
                    if (NotExistInGroup(Group1, MergeResult))
                        Group1.push_back(MergeResult);
                }
                j++;
            }
            if (!Group1[0].mark)
                PutInPrimeImplicant(Group1[0]);
            Group1.erase(Group1.begin());
        }
    }
    void ProcessLastGroup() // Process last group and delete any empty groups
    {
        while (!Minterms.back().empty())
        {
            if (!Minterms.back()[0].mark)
                PutInPrimeImplicant(Minterms.back()[0]);
            Minterms.back().erase(Minterms.back().begin());
        }
        Minterms.pop_back();
        int size = Minterms.size();
        for (int i = 0; i < size;)
        {
            if (Minterms[i].empty())
            {
                Minterms.erase(Minterms.begin() + i);
                size--;
            }
            else
                i++;
        }
    }
};
struct PetrickMethod
{
    map<int, int> &OnSet;
    vector<Term> &PrimeImplicant;

    int CountLiteral(set<string> &s)
    {
        int count = 0;
        for (const auto &it1 : s)
            for (const auto &it2 : it1)
                if (it2 != '-')
                    count++;
        return count;
    }
    static bool sortP1P2(const set<string> a, const set<string> b)
    {
        return (a.size() < b.size());
    }
    void Absorption(vector<set<string>> &MergeResult, set<string> &P1P2)
    {
        sort(MergeResult.begin(), MergeResult.end(), sortP1P2);
        int size = MergeResult.size();
        for (int i = 0; i < size - 1; i++)
        {
            for (int j = i + 1; j < size;)
            {
                int count = 0;
                for (auto itI = MergeResult[i].begin(); itI != MergeResult[i].end(); itI++)
                {
                    if (MergeResult[j].find(*itI) == MergeResult[j].end())
                        break;
                    else
                        count++;
                }
                if (count == MergeResult[i].size())
                {
                    MergeResult.erase(MergeResult.begin() + j);
                    size--;
                }
                else
                    j++;
            }
        }
    }
    vector<set<string>> Merge(vector<set<string>> &P1P2_Set, set<string> &P1P2)
    {
        vector<set<string>> MergeResult;
        for (const auto &itS : P1P2_Set)
        {
            for (const auto &it2 : P1P2)
            {
                set<string> p1p2;
                p1p2.insert(it2);
                for (const auto &it1 : itS)
                    p1p2.insert(it1);
                MergeResult.push_back(p1p2);
            }
        }
        Absorption(MergeResult, P1P2);
        return MergeResult;
    }
    set<string> FindMinCover()
    {
        vector<set<string>> P1P2_Set;
        auto itO = OnSet.begin();
        for (const auto &itP : PrimeImplicant)
        {
            bool flag = false;
            set<string> P1P2;
            if (itP.PI_chart[itO->second])
            {
                flag = true;
                P1P2.insert(itP.value);
            }
            if (flag)
                P1P2_Set.push_back(P1P2);
        }
        itO++;
        for (; itO != OnSet.end(); itO++)
        {
            set<string> P1P2;
            for (const auto &itP : PrimeImplicant)
                if (itP.PI_chart[itO->second])
                    P1P2.insert(itP.value);
            P1P2_Set = Merge(P1P2_Set, P1P2);
        }
        int MinLiteral = 100000000, i = 0, MinIndex = 0;
        for (auto &it : P1P2_Set)
        {
            int literal = CountLiteral(it);
            if (MinLiteral > literal)
            {
                MinLiteral = literal;
                MinIndex = i;
            }
            i++;
        }
        return P1P2_Set[MinIndex];
    }
};
struct Output
{
    const int &Variables;
    vector<Term> &PrimeImplicant;
    set<string> &MinCover;
    string &inFile;
    string &outFile1;
    string &outFile2;

    static bool SortByLiteral(const Term a, const Term b)
    {
        int size = a.value.length(), countA = 0, countB = 0;
        for (int i = 0; i < size; i++)
        {
            if (a.value[i] != '-')
                countA++;
            if (b.value[i] != '-')
                countB++;
        }
        return (countA < countB);
    }
    void OutputImplicant()
    {
        string s;
        ifstream ifs(inFile, ios::in);
        ofstream ofs(outFile1, ios::out);
        if (!ofs)
        {
            cerr << outFile1 << " could not be opened!" << endl;
            exit(1);
        }
        if (!ifs)
        {
            cerr << inFile << " could not be opened!" << endl;
            exit(1);
        }
        while (!ifs.eof())
        {
            getline(ifs, s);
            ofs << s << endl;
        }
        ifs.close();
        ofs << ".p " << PrimeImplicant.size() << endl;
        int i = 0;
        sort(PrimeImplicant.begin(), PrimeImplicant.end(), SortByLiteral);
        for (const auto &it : PrimeImplicant)
        {
            if (i++ == 20)
                break;
            ofs << it.value << endl;
        }
        ofs.close();
    }
    int CountLiteral()
    {
        int count = 0;
        for (const auto &it1 : MinCover)
            for (const auto &it2 : it1)
                if (it2 != '-')
                    count++;
        return count;
    }
    void OutputMinCover()
    {
        ofstream ofs(outFile2, ios::out);
        if (!ofs)
        {
            cerr << outFile2 << " could not be opened!" << endl;
            exit(1);
        }
        ofs << ".mc " << MinCover.size() << endl;
        for (const auto &it : MinCover)
            ofs << it << endl;
        ofs << "literal=" << CountLiteral();
        ofs.close();
    }
};

int main(int argc, char **argv)
{
    string inFile, outFile1, outFile2;
    SetFileName(argc, argv, inFile, outFile1, outFile2);

    int Variables;
    vector<int> DontCareSet;
    map<int, int> OnSet;

    // Read input.txt
    ReadFile_and_Initial(inFile, Variables, OnSet, DontCareSet);
    vector<vector<Term>> Minterms(Variables + 1);
    SetMinterms(Variables, OnSet, DontCareSet, Minterms);

    // Compute start
    vector<Term> PrimeImplicant;
    QM qm = {Variables, OnSet, Minterms, PrimeImplicant};
    while (!Minterms.empty())
    {
        int size = Minterms.size();
        for (int i = 0; i < size - 1; i++)
            qm.PairInGroups(Minterms[i], Minterms[i + 1]);
        qm.ProcessLastGroup();
    }
    // QM complete

    // Generate implicant.txt
    set<string> MinCover;
    Output out = {Variables, PrimeImplicant, MinCover, inFile, outFile1, outFile2};

    out.OutputImplicant();
    // Petrick's method start
    PetrickMethod pm = {OnSet, PrimeImplicant};
    MinCover = pm.FindMinCover();
    // PetrickMethod complete

    // Generate output.txt
    out.OutputMinCover();

    // cout << "Excution time:" << (t2 - t0) / CLOCKS_PER_SEC << " sec." << endl; // for debug
    return 0;
}