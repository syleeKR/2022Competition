
#pragma GCC optimize ("O3")
#pragma GCC optimize ("Ofast")
#pragma GCC optimize ("unroll-loops")
#pragma GCC target("sse,sse2,sse3,ssse3,sse4,avx,avx2")
#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef long double ld;
const ll mod = 1e9+7;
const int MxN = 1e5;
const int inf = INT_MAX;
#define REP0(i,n) for(int i=0; i<n; i++)
#define REP1(i,n) for(int i=1; i<=n; i++)
#define REP(i,a,b) for(int i=a; i<=b; i++)
#define sz(v) ((int)(v).size())
#define all(v) (v).begin(), (v).end()
#define compress(v) sort(all(v)); v.erase(    unique(all(v)) , v.end()   )
#define reset(X) memset(X, 0, sizeof(X))
#define pb push_back
#define fi first
#define se second
#define pii pair<int, int>
#define pll pair<ll, ll>
#define vint vector<int>
#define vll vector<ll>
#define vpii vector<pair<int, int>>
#define vpll vector<pair<ll,ll>>
#define LOG 18

///////////////////////////////////////////Todo//////////////////////////////////////////////
//1.n=2인 경우에서 지금은 x, x+1 꼴만 확인하는데 이거 바꿔야함(n=4일때는 달라져야지...)
//2.더 efficient하게 돌리기(그냥 random permutation이 아니라 rack 고정하고 permutation 돌려야 될듯?)
// 3. 일단 가능한 하나의 rack에 모아줘야 될듯


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// important variables ////////////////////////////////////////////


int nNetwork, nRack, nPm, nNuma; 

vector<int> permutation;
vector<int> permutation2;
vector<int> permutation3;
int nTotal;


// nodeinfo vector stores the information about the ith numanode.
struct Numanode
{
    int cpu;
    int memory;
};
vector<Numanode> nodeinfo;


//vmtype vector stores the information about ith VM type
struct VMinfo
{
    int cpu;
    int memory;
    int numacount;
};
vector<VMinfo> vmtype;


// pgtype vector stores the information about the constraints of ith pg
struct PGinfo
{
    int index;
    int npartition; //0 <= partition <= 7   (partition on "rack"    0: no, else:hard)
    int maxperPM;   //0  <= maxperPM <= 5  (0 : no, else:soft)
    int n_affinity; // 0(no constraint) , 1(soft), 2(hard)
    int r_affinity;  //0 (no constraint), 1(soft), 2(hard)
      
};
vector<PGinfo> pgtype(1);
int memorynumofpg[13502]; //for n_affinity, it has to be in the same memory



// inside vm[i], you can access the info(VMinfo), pg(pginfo), index, and the partition it is in.
struct VM
{
    int index;
    PGinfo pg;
    VMinfo info; 
    int partition;
};
vector<VM> vm(1);   


// each component of a 4 dimensional vector is a set of integers
// storage[network][rack][pm][numa] : set of integers representing the Vms
vector<vector<vector<vector<  set<int>    >>>> storage;
vector<vector<vector<vector<     Numanode      >>>> leftover;
// family[i] : all indices of vm in ith pg.
// set<int> family[13501];


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// delte algorithms /////////////////////////////////////////////////

vector<int> findVM(int index)
{
    // given the index of vm, find the network , rack, pm, numanode
    // numanode can be 1 or 2 depending on vm[index].info.numcount
    vector<int> temp;
    int found = 0;    
    REP1(i, nNetwork)
    {
        REP1(j, nRack)
        {
            REP1(k, nPm)
            {
                REP1(l, nNuma)
                {
                    if( storage[i][j][k][l].count(index))
                    {
                        if(found==0)
                        {
                            temp.pb(i);temp.pb(j); temp.pb(k); temp.pb(l);
                            found++;
                        }
                        else
                        {
                            temp.pb(l);
                            found++;
                        }
                    }
                }
                if(found!=0)
                {
                    break;
                }

            }
        }
    }
    return temp;
}


void deleteVM(int index)
{
    // delete the particular vm from storage
    // we need to keep track of storage, leftover, and family
    vint pos = findVM(index);
    int pgindex = vm[index].pg.index;
    if(sz(pos)==4)
    {
        storage[pos[0]][pos[1]][pos[2]][pos[3]].erase(index);
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].memory += vm[index].info.memory;
        
    }
    else
    {
        storage[pos[0]][pos[1]][pos[2]][pos[3]].erase(index);
        storage[pos[0]][pos[1]][pos[2]][pos[4]].erase(index);

        leftover[pos[0]][pos[1]][pos[2]][pos[3]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].memory += vm[index].info.memory;

        leftover[pos[0]][pos[1]][pos[2]][pos[4]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[4]].memory += vm[index].info.memory;
    }
    //family[pgindex].erase(index);

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// allocation1 algorithms ///////////////////////////////////////////



vector<int> naivefill1(VM vm0)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation));
    REP0(i, nTotal)
    {
        int numa_index = permutation[i]%nNuma;
        int pm_index = permutation[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = permutation[i] % (nTotal/nNetwork) / (nNuma*nPm); 
        int network_index  = permutation[i] / (nTotal/nNetwork);

        numa_index ++;
        pm_index ++;
        rack_index ++;
        network_index ++;

        Numanode & target = leftover[network_index][rack_index][pm_index][numa_index];  
        set<int> &s = storage[network_index][rack_index][pm_index][numa_index];   

        if(target.cpu >= cpu && target.memory >= memory)
        {
            bool problem =false;

            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target.cpu -= cpu;
                target.memory -= memory;

                ans = {network_index, rack_index, pm_index, numa_index};
                // update target
                s.insert(vm0.index);
                break;
            }
            

        }


    }

    return ans;


}

vector<int> naivefill1_network(VM vm0, int network)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation2));
    REP0(i, nTotal/nNetwork)
    {
        int numa_index = permutation2[i]%nNuma;
        int pm_index = permutation2[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = permutation2[i] % (nTotal/nNetwork) / (nNuma*nPm); 
        int network_index  =network;

        numa_index ++;
        pm_index ++;
        rack_index ++;

        Numanode & target = leftover[network_index][rack_index][pm_index][numa_index];  
        set<int> &s = storage[network_index][rack_index][pm_index][numa_index];  

        if(target.cpu >= cpu && target.memory >= memory)
        {
            bool problem =false;

            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target.cpu -= cpu;
                target.memory -= memory;

                ans = {network_index, rack_index, pm_index, numa_index};
                // update target
                s.insert(vm0.index);
                break;
            }
            

        }


    }

    return ans;


}

vector<int> naivefill1_rack(VM vm0, int network, int rack)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation3));
    REP0(i, nTotal/nNetwork/nRack)
    {
        int numa_index = permutation[i]%nNuma;
        int pm_index = permutation[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = rack;
        int network_index  = network;

        numa_index ++;
        pm_index ++;

        Numanode & target = leftover[network_index][rack_index][pm_index][numa_index];   
        set<int> &s = storage[network_index][rack_index][pm_index][numa_index];   

        if(target.cpu >= cpu && target.memory >= memory)
        {
            bool problem =false;
            
            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target.cpu -= cpu;
                target.memory -= memory;

                ans = {network_index, rack_index, pm_index, numa_index};
                // update target
                s.insert(vm0.index);
                break;
            }
            

        }


    }

    return ans;


}



bool allocate1(vector<VM> & vms, vector< vector<int> > & answer)
{
    // 0 . vm, answer are zero based
    // 1.update the storage, leftover
    // 2. udate answer vector
    // 3. return possibility

    //vms shares the same vm property and are in same pg!!(might have different partition)

    int n = sz(vms);

    //shared parameter of n vms
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int numacount = vms[0].info.numacount;

    int raffinity = vms[0].pg.r_affinity;
    int naffinity = vms[0].pg.n_affinity;
    
    //lets just consider the hard constraints
    //hard constraints can be
    // 1. hard rack affinity
    // 2. hard network affinity
    // 3. hard rack anti affinity with partition(same partition cannot be in sam rack) 

    bool possible =true;

    if(raffinity == 2)
    {
        possible =false;

        for(int network = 1; network<=nNetwork;network++)
        {
            for(int rack=1; rack<=nRack; rack++)
            {
            bool breaked = false;
            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill1_rack(x, network,rack);
            
                if(sz(v)==0)
                {
                    
                    breaked=true;
                    // 되돌리기
                    for(int j=0; j<i;j++)
                    {
                        int network_index = answer[j][0];
                        int rack_index = answer[j][1];
                        int pm_index = answer[j][2];
                        int numa_index = answer[j][3];
                        Numanode & target = leftover[network_index][rack_index][pm_index][numa_index];   
                        set<int> &s = storage[network_index][rack_index][pm_index][numa_index]; 
                        target.cpu += cpu;
                        target.memory += memory;
                        s.erase(vms[j].index);
                    }
                    break;
                }

                answer[i] = v;

            }
            if(breaked==false)
            {
                possible=true;
                break;
            }
            }
            if(possible==true)break;
               
        }

        
    }
    else if(naffinity==2)
    {

        int predefined = memorynumofpg[vms[0].pg.index];
        if(predefined==0){
            possible=false;
        for(int network = 1; network<=nNetwork;network++)
        {
            bool breaked = false;
            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill1_network(x, network);
            
                if(sz(v)==0)
                {

                    // 되돌리기
                    for(int j=0; j<i;j++)
                    {
                        int network_index = answer[j][0];
                        int rack_index = answer[j][1];
                        int pm_index = answer[j][2];
                        int numa_index = answer[j][3];
                        Numanode & target = leftover[network_index][rack_index][pm_index][numa_index];   
                        set<int> &s = storage[network_index][rack_index][pm_index][numa_index]; 
                        target.cpu += cpu;
                        target.memory += memory;
                        s.erase(vms[j].index);
                    }
                    
                    breaked=true;
                    break;
                }

                answer[i] = v;

            }
            if(breaked==false)
            {
                //이제 이 pg의 vm들은 반드시 이 network에 들어와야함.
                memorynumofpg[vms[0].pg.index] = network;
                possible=true;
                break;
            }
               
        }
        }
        else{
            int network = predefined;

            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill1_network(x, network);
            
                if(sz(v)==0)
                {
                    possible = false;
                    break;
                }

                answer[i] = v;

            }

        }

    }
    else{
        REP0(i, n)
        {
            VM x = vms[i];

            vint v = naivefill1(x);
            if(sz(v)==0)
            {
                possible=false;
                break;
            }
            answer[i] = v;
        }
    }

    

    

    return possible;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// allocation2 algorithms ///////////////////////////////////////////


vector<int> naivefill2(VM vm0)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation));
    REP0(i, nTotal)
    {
        int numa_index = permutation[i]%nNuma;
        int pm_index = permutation[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = permutation[i] % (nTotal/nNetwork) / (nNuma*nPm); 
        int network_index  = permutation[i] / (nTotal/nNetwork);

        numa_index ++;
        pm_index ++;
        rack_index ++;
        network_index ++;

        Numanode & target1 = leftover[network_index][rack_index][pm_index][numa_index];  
        set<int> &s1 = storage[network_index][rack_index][pm_index][numa_index];   

        Numanode & target2 = leftover[network_index][rack_index][pm_index][numa_index%nNuma+1];   
        set<int> &s2 = storage[network_index][rack_index][pm_index][numa_index%nNuma+1];   

        if(target1.cpu >= cpu && target1.memory >= memory && target2.cpu >= cpu && target2.memory >= memory)
        {
            bool problem =false;

            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target1.cpu -= cpu;
                target1.memory -= memory;
                target2.cpu -= cpu;
                target2.memory -= memory;

                ans = {network_index, rack_index, pm_index, min(numa_index, numa_index%nNuma+1), max(numa_index, numa_index%nNuma+1)};
                // update target
                s1.insert(vm0.index);
                s2.insert(vm0.index);

                break;
            }
            

        }


    }

    return ans;


}

vector<int> naivefill2_network(VM vm0, int network)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation2));
    REP0(i, nTotal/nNetwork)
    {
        int numa_index = permutation2[i]%nNuma;
        int pm_index = permutation2[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = permutation2[i] % (nTotal/nNetwork) / (nNuma*nPm); 
        int network_index  =network;

        numa_index ++;
        pm_index ++;
        rack_index ++;

        Numanode & target1 = leftover[network_index][rack_index][pm_index][numa_index];  
        set<int> &s1 = storage[network_index][rack_index][pm_index][numa_index];   

        Numanode & target2 = leftover[network_index][rack_index][pm_index][numa_index%nNuma+1];   
        set<int> &s2 = storage[network_index][rack_index][pm_index][numa_index%nNuma+1];   

        if(target1.cpu >= cpu && target1.memory >= memory && target2.cpu >= cpu && target2.memory >= memory)
        {
            bool problem =false;
            
            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target1.cpu -= cpu;
                target1.memory -= memory;
                target2.cpu -= cpu;
                target2.memory -= memory;

                ans = {network_index, rack_index, pm_index, min(numa_index, numa_index%nNuma+1), max(numa_index, numa_index%nNuma+1)};
                // update target
                s1.insert(vm0.index);
                s2.insert(vm0.index);

                break;
            }
            

        }


    }

    return ans;


}

vector<int> naivefill2_rack(VM vm0, int network, int rack)   //only need to satisfy cpu, memory condition
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int numacount = vm0.info.numacount; // =1
    int partition = vm0.partition;
    int pg = vm0.pg.index;

    vint ans = {};

    random_shuffle(all(permutation3));
    REP0(i, nTotal/nNetwork/nRack)
    {
        int numa_index = permutation[i]%nNuma;
        int pm_index = permutation[i]%(nTotal/nNetwork/nRack) / (nNuma);
        int rack_index = rack;
        int network_index  = network;

        numa_index ++;
        pm_index ++;

        Numanode & target1 = leftover[network_index][rack_index][pm_index][numa_index];  
        set<int> &s1 = storage[network_index][rack_index][pm_index][numa_index];   

        Numanode & target2 = leftover[network_index][rack_index][pm_index][numa_index%nNuma+1];   
        set<int> &s2 = storage[network_index][rack_index][pm_index][numa_index%nNuma+1];   

        if(target1.cpu >= cpu && target1.memory >= memory && target2.cpu >= cpu && target2.memory >= memory)
        {
            bool problem =false;
            
            REP1(iter1, nPm){REP1(iter2, nNuma){
            set<int> sprime = storage[network_index][rack_index][iter1][iter2];
            for(auto x : sprime)
            {
                if(vm[x].pg.index == pg && vm[x].pg.npartition!=0 && vm[x].partition!=partition)
                {
                    problem = true;
                    break;
                }
            }
            if(problem)break;
            }
            if(problem)break;
            }

            if(!problem)
            {
                // update leftover
                target1.cpu -= cpu;
                target1.memory -= memory;
                target2.cpu -= cpu;
                target2.memory -= memory;

                ans = {network_index, rack_index, pm_index, min(numa_index, numa_index%nNuma+1), max(numa_index, numa_index%nNuma+1)};
                // update target
                s1.insert(vm0.index);
                s2.insert(vm0.index);

                break;
            }
            

        }


    }

    return ans;


}


bool allocate2(vector<VM> & vms, vector< vector<int> > & answer)
{
    // 0 . vm, answer are zero based
    // 1.update the storage
    // 2. udate answer vector
    // 3. return possibility

    //vms shares the same vm property and are in sam pg!!(might have different partition)

    int n = sz(vms);

    //shared parameter of n vms
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int numacount = vms[0].info.numacount;

    int raffinity = vms[0].pg.r_affinity;
    int naffinity = vms[0].pg.n_affinity;
    
    //lets just consider the hard constraints
    //hard constraints can be
    // 1. hard rack affinity
    // 2. hard network affinity
    // 3. hard rack anti affinity with partition(same partition cannot be in sam rack) 

    bool possible =true;

    if(raffinity == 2)
    {
        possible =false;

        for(int network = 1; network<=nNetwork;network++)
        {
            for(int rack=1; rack<=nRack; rack++)
            {
            bool breaked = false;
            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill2_rack(x, network,rack);
            
                if(sz(v)==0)
                {
                    for(int j=0; j<i;j++)
                    {
                        int network_index = answer[j][0];
                        int rack_index = answer[j][1];
                        int pm_index = answer[j][2];
                        int numa_index1 = answer[j][3];
                        int numa_index2 = answer[j][4];
                        Numanode & target1 = leftover[network_index][rack_index][pm_index][numa_index1];   
                        set<int> &s1 = storage[network_index][rack_index][pm_index][numa_index1]; 
                        target1.cpu += cpu;
                        target1.memory += memory;
                        s1.erase(vms[j].index);
                        Numanode & target2 = leftover[network_index][rack_index][pm_index][numa_index2];   
                        set<int> &s2 = storage[network_index][rack_index][pm_index][numa_index2]; 
                        target2.cpu += cpu;
                        target2.memory += memory;
                        s2.erase(vms[j].index);
                    }
                    breaked=true;
                    break;
                }

                answer[i] = v;

            }
            if(breaked==false)
            {
                possible=true;
                break;
            }
            }
            if(possible==true)break;
               
        }

        
    }
    else if(naffinity==2)
    {

        int predefined = memorynumofpg[vms[0].pg.index];

        if(predefined==0){
            possible = false;
        for(int network = 1; network<=nNetwork;network++)
        {
            bool breaked = false;
            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill2_network(x, network);
            
                if(sz(v)==0)
                {
                    for(int j=0; j<i;j++)
                    {
                        int network_index = answer[j][0];
                        int rack_index = answer[j][1];
                        int pm_index = answer[j][2];
                        int numa_index1 = answer[j][3];
                        int numa_index2 = answer[j][4];
                        Numanode & target1 = leftover[network_index][rack_index][pm_index][numa_index1];   
                        set<int> &s1 = storage[network_index][rack_index][pm_index][numa_index1]; 
                        target1.cpu += cpu;
                        target1.memory += memory;
                        s1.erase(vms[j].index);
                        Numanode & target2 = leftover[network_index][rack_index][pm_index][numa_index2];   
                        set<int> &s2 = storage[network_index][rack_index][pm_index][numa_index2]; 
                        target2.cpu += cpu;
                        target2.memory += memory;
                        s2.erase(vms[j].index);
                    }
                    breaked=true;
                    break;
                }

                answer[i] = v;

            }
            if(breaked==false)
            {
                memorynumofpg[vms[0].pg.index] = network;

                possible=true;
                break;
            }
               
        }
        }
        else{
            int network = predefined;

            REP0(i, n)
            {
                VM x = vms[i];

                vint v;

            
                
                v = naivefill2_network(x, network);
            
                if(sz(v)==0)
                {
                    possible = false;
                    break;
                }

                answer[i] = v;

            }

        }

    }
    else{
        REP0(i, n)
        {
            VM x = vms[i];

            vint v = naivefill2(x);
            if(sz(v)==0)
            {
                possible=false;
                break;
            }
            answer[i] = v;
        }
    }

    

    

    return possible;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// three types of requests ////////////////////////////////////////


void PG_creation()
{
    //new pg is described. We will store it into pgtype vector
    int index; cin>>index;
    int partition ;  cin>>partition;
    int maxperPM; cin>>maxperPM;
    int network_affinity; cin>>network_affinity;
    int rack_affinity; cin>>rack_affinity;

    PGinfo placement_group;
    placement_group.index = index;
    placement_group.npartition = partition;
    placement_group.maxperPM = maxperPM;
    placement_group.n_affinity = network_affinity;
    placement_group.r_affinity = rack_affinity;

    pgtype.pb(placement_group);
}

bool VM_creation()
{
    // how many vms to be created
    int ncreate; cin>>ncreate;
    
    // type of vm, type of pg, the partition of pg;
    int typeofvm ; cin>>typeofvm;
    int indexofpg; cin>>indexofpg;
    int partition; cin>>partition;

    //read in ncreate number of new vms, store it in vm vector and temp vector
    vector<VM> temp;
    REP1(i,ncreate)
    {
        VM newvm;
        
        cin >> newvm.index;

        newvm.info  = vmtype[typeofvm];
        newvm.pg = pgtype[indexofpg]; 
        newvm.partition = (partition!=-1? partition : i);

        temp.pb(newvm);
    }

    // check if we can allocate the new Vms and if so, print it out!
    // We use two functions each for numacount=1 and numacount=2
    //return whether it is possible or not
    bool possible;
    if (vmtype[typeofvm].numacount ==1 )
    {
        vector<int> v(4,0);
        vector<vector<int>> answer(ncreate, v);
        possible = allocate1(temp, answer);

        if(possible)
        {
            // update vm
            for (auto x : temp)vm.pb(x);
            
            //print out allocation
            REP0(i, ncreate)
            {
                REP0(j,4)
                {
                    cout<<answer[i][j]<<" ";
                }
                cout<<endl;
            }
        }
        else cout<<-1<<endl;

    }
    else{
        vector<int> v(5,0);
        vector<vector<int>> answer(ncreate, v);
        possible = allocate2(temp, answer);

        if(possible)
        {
            // update vm
            for (auto x : temp)vm.pb(x);
            
            //print out allocation
            REP0(i, ncreate)
            {
                REP0(j,5)
                {
                    cout<<answer[i][j]<<" ";
                }
                cout<<endl;
            }
        }
        else cout<<-1<<endl;
    }
    

    return possible;
}


void VM_deletion()
{
    // we are going to delete ndelte virtual machines!
    int ndelete; cin>>ndelete;

    //they are guaranteed to be from same PG, they are not deleted before

    REP1(i, ndelete)
    {
        int index; cin>>index;
        deleteVM(index);
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// main function and basic stuff //////////////////////////////////////////



void make()
{
    // everything is "1"based indexing
    // every storage[network][rack][pm][numanode]  is a "set" of integers
    set<int> onenuma;
    vector<set<int>> onepm(nNuma+1, onenuma);
    vector<vector<set<int>>> onerack(nPm+1, onepm);
    vector<vector<vector<set<int>>>> onenetwork(nRack+1, onerack);
    storage.resize(nNetwork+1, onenetwork);

    vector<Numanode> onepm2 =  nodeinfo;
    vector<vector<Numanode>> onerack2(nPm+1, onepm2);
    vector<vector<vector<Numanode>>> onenetwork2(nRack+1, onerack2);
    leftover.resize(nNetwork+1, onenetwork2);
    

}

void createpermutation()
{
    nTotal =  nNetwork*nRack*nPm*nNuma;
    REP0(i, nNetwork*nRack*nPm*nNuma)permutation.pb(i);

    REP0(i, nRack*nPm*nNuma)permutation2.pb(i);


    REP0(i, nPm*nNuma)permutation3.pb(i);



}


void read_basic()
{
    //read in basic stuff like number of network, racks, PMs, NUMA nodes;
    cin>>nNetwork>>nRack>>nPm>>nNuma;

    createpermutation();

 
    //read in information of each numa nodes
    nodeinfo.resize(nNuma+1);
    REP1(i, nNuma)cin >> nodeinfo[i].cpu >> nodeinfo[i].memory;   // numanode[i].cpu, numanode[i].memory

    //read in information of vms (the type of vms)
    int nVmtype; cin>>nVmtype; //number of type of vm
    vmtype.resize(nVmtype+1);
    REP1(i, nVmtype)cin >> vmtype[i].numacount >> vmtype[i].cpu>>vmtype[i].memory;   // vm[i].numacount, vm[i].cpu, vm[i].memory

    //make storage. storage allows us to keep track of the location of Vms
    //make leftover. it allows us to keep track of the cpu, memory 
    make();

}

int main()
{
    //efficient cin,cout
    ios::sync_with_stdio(0);

    //read basic stuff
    read_basic();

    //read in request and handle (loop until termination(4))
    //request < 13500 , VMs < 235000

    int request_type; cin>>request_type;
    while(request_type !=4 )
    {

        if(request_type==1)
        {
            //create placement group
            PG_creation();
        }
        else if(request_type == 2)
        {
            //create VMS
            bool possible = VM_creation();
            if(possible==false)break;
        }
        else
        {
            //delete VM
            VM_deletion();
        }
        cin>> request_type;
    }
}
