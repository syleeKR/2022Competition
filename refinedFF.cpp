
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
//#define endl "\n"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////current status///////////////////////////////////////////////////

//"distribute network , FF on individual network"
//Why? 원래라면 network도  1부터 FF형식으로 배치하는 것이 이득.
// 그러나 "같은 network에 들어가야 합니다"의 affinity 조건이 존재하기에 network는 최대한 분배해주는것이 좋음
// rack의 경우 "같은 rack에 들어가야 합ㄴ디ㅏ"의 affinity 조건이 존재해도 한번만 사용되기 때문에 ok. 

//partition 의 문제를 해결하는 방법은?


//////////////////////////////////////control experiment settings//////////////////////////////////////////// 
bool debugmode = false;

int timecutoff = 20000;
int vmcutoff  = 300000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// important variables ////////////////////////////////////////////

int nNetwork, nRack, nPm, nNuma; 
int nTotal;
int vmtotal = 0;


// nodeinfo vector stores the information about the ith numanode.
struct Numanode
{
    int cpu;
    int memory;
};
vector<Numanode> nodeinfo;

vector<int> partner;

//vmtype vector stores the information about ith VM type
struct VMinfo
{
    int cpu;
    int memory;
    int numacount;
};
vector<VMinfo> vmtype;

int startingpm[21];
bool specialvm[21];

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
int networknumofpg[13502]; //for n_affinity, it has to be in the same network


// inside vm[i], you can access the info(VMinfo), pg(pginfo), index, and the partition it is in.
struct VM
{
    int index;
    PGinfo pg;
    VMinfo info; 
    int partition;
    vint loc;
    int type;
};
vector<VM> vm(1);   


// each component of a 4 dimensional vector is a set of integers
// storage[network][rack][pm][numa] : set of integers representing the Vms
vector<vector<vector<vector<  set<int>    >>>> storage;
vector<vector<vector<vector<     Numanode      >>>> leftover;

int partition_storage[9][33][13501][8];

/////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// some supplementary algos ///////////////////////////////////////

//for finding out conditions that terminate request
void print(int request)
{
    
    switch (request)
    {
        case 1:
            cout<<"a"<<endl;
            //cout<<"rack affinity condition : cannot allocate into a single rack"<<endl;
            break;
        case 2:
            cout<<"b"<<endl;
            //cout<<"network affinity condition undefined : cannot allocate into a single network"<<endl;
            break;
        case 3:
            cout<<"c"<<endl;
            //cout<<"network affinity condition undefined : cannot allocate into a single network,different racks"<<endl;
            break;
        case 4:
            cout<<"d"<<endl;
            //cout<<"network affinity condition predefined : cannot allocate into a single network"<<endl;
            break;
        case 5:
            cout<<"e"<<endl;
            //cout<<"network affinity condition predefined : cannot allocate into a single network, considering partition"<<endl;
            break;
        case 6:
            cout<<"f"<<endl;
            //cout<<"network affinity condition predefined : cannot allocate into a single network, different racks"<<endl;
            break;
        case 7:
            cout<<"g"<<endl;
            //cout<<"no affinity condition : cannot allocate into a single resource"<<endl;
            break;
        case 8:
            cout<<"h"<<endl;
            //cout<<"no affinity condition : cannot allocate into a single resource, considering partition"<<endl;
            break;
        case 9:
            cout<<"i"<<endl;
            //cout<<"no affinity condition : cannot allocate into a single resource, different racks"<<endl;
            break;

    }
    
    
    
}

//set value of partner vector
void setval(int a, int b, int c, int d)
{
    partner[1] = a;
    partner[2] = b;
    partner[3] = c;
    partner[4] = d;
}
//print 2d vector of ints
void print2d(vector<vint> & v)
{
    int n = sz(v);
    int m = sz(v[0]);

    REP0(i,n)
    {
        REP0(j,m)
        {
            cout<<v[i][j]<<" ";
        }
        cout<<endl;
    }
}

//next index considering circular relation
void next(int & a, int N)
{
    a=a+1;
    if(a>N)a=1;
}


vint permutation;   // 0~ nPm * nNuma-1 가 한번씩 나오는 순열
void preprocess()
{
    int pminr = nNuma * nPm;
    permutation.resize(pminr);
    REP0(i, pminr)permutation[i]=i;
}

// 0 ~ nPm * nNuma-1 에서 {pm_index, numa_index}로 가는 1-1 대응 
pii numtoloc(int x)
{
    int rack_index = x/nNuma; rack_index++;
    int numa_index = x % nNuma; numa_index++;
    return {rack_index, numa_index};
}  

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Functions answering a query ///////////////////////////////////////////

// determine wheter we can allocate a vm of partition p in this rack (in terms of partition)
//O(#VM : 235000)
bool partitioncheck(int network, int rack, int pgindex, int p)
{
    for(int i=1; i<=7;i++)
    {
        if(i==p)continue;
        if(partition_storage[network][rack][pgindex][i]!=0)return false;
    }
    return true;
}


// determine if the rack contains vm with the particular partition p != 0
// O(#VM :  235000)
bool exist(int pgindex, int p, int network, int rack)
{
    if(partition_storage[network][rack][pgindex][p]!=0)return true;
    return false;
}


//decide whether we can allocate vm0 in the given position
// consider every condition
// consider cpu,memory
// consider type
bool canallocate(VM &vm0, int network, int rack, int pm, int numa)
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;
    int pgindex = vm0.pg.index;
    int npartition = vm0.pg.npartition;

    Numanode l1  = leftover[network][rack][pm][numa];
    Numanode l2  = leftover[network][rack][pm][partner[numa]];
    if(type==1)
    {
        if(cpu<=l1.cpu && memory <= l1.memory)return true;
        else return false;
    }
    else
    {
        if(cpu <= l1.cpu && memory<=l1.memory && cpu<=l2.cpu && memory <= l2.memory)return true;
        return false;
    }
}

int fullyallocate(VM &vm0, int network, int rack, int pm, int numa)
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;
    
    
    //cpu, memory +(type) condition
    Numanode l1  = leftover[network][rack][pm][numa];
    Numanode l2  = leftover[network][rack][pm][partner[numa]];
    if(type==1)
    {
        return min(l1.cpu/cpu, l1.memory/memory);
    }
    else
    {
        return min( min(l1.cpu/cpu , l1.memory/memory) , min(l2.cpu/cpu , l2.memory/memory));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// 'Finding rack' algorithms /////////////////////////////////////////////////////
//////////////////////////////// Find all racks that have vm with partition p 
//////////////////////////////// Find all racks where vm with partition p can be placed in.

// find all racks that have vms that are in pg : pgindex and partition : p
vector<pii> findallrack(int pgindex, int p)
{
    vector<pii> racks;
    
    
    for (int network_index=1; network_index<=nNetwork; network_index++)
        {
            for (int rack_index=1; rack_index<=nRack; rack_index++)
    {
                 
            if(exist(pgindex, p, network_index, rack_index))
            {
                racks.pb({network_index, rack_index});
            }
            
        }
    }
    
    vector<vpii> temp(nRack+1);
    for (auto x : racks)temp[x.se].pb(x);
    vpii answer;
    for(auto x : temp)
    {
        for(auto y : x)answer.pb(y);
    }
    return answer;

}

vector<pii> findallrack(int pgindex, int p, int network)
{
    vector<pii> racks;
    
        for (int rack_index=1; rack_index<=nRack; rack_index++)
        {
            if(exist(pgindex, p, network, rack_index))
            {
                racks.pb({network, rack_index});
            }
            
        }
    
    return racks;
}

// find all racks that are possible to use in terms of partition
vector<pii> possiblerack(int pgindex, int p)
{
    vector<pii> racks;
    
    
    for (int network_index=1; network_index<=nNetwork; network_index++)
    {
        for (int rack_index=1; rack_index<=nRack; rack_index++)
        {
    
            if(partitioncheck( network_index, rack_index,pgindex,p))
            {
                racks.pb({network_index, rack_index});
            }
            
        }
    }
    
    vector<vpii> temp(nRack+1);
    for (auto x : racks)temp[x.se].pb(x);
    vpii answer;
    for(auto x : temp)
    {
        for(auto y : x)answer.pb(y);
    }
    return answer;
}

vector<pii> possiblerack(int pgindex, int p, int network)
{
    vector<pii> racks;
    int network_index = network;
        for (int rack_index=1; rack_index<=nRack; rack_index++)
        {
            if(partitioncheck( network_index, rack_index,pgindex,p))
            {
                racks.pb({network_index, rack_index});
            }
            
        }
    
    return racks;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// maxallocate /////////////////////////////////////////////////////////
////////////////// Determine the maximum number of virtual machine we can allocate right now ///////////////

int maxallocate_candidates(vector<VM> & vms, vpii & candidates)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;

    //get all possiblerack considering constraint
    
    if(type==1)
    {
        for(auto x : candidates)
        {
            
            for(int pm_index = 1; pm_index <=nPm; pm_index++)
            {
                for(int numa_index =1; numa_index <= nNuma; numa_index++)
                {
                    int leftovercpu = leftover[x.fi][x.se][pm_index][numa_index].cpu;
                    int leftovermemory = leftover[x.fi][x.se][pm_index][numa_index].memory;
                    possiblevm += min(leftovercpu/cpu , leftovermemory/memory);
                }
            }
        }
        return possiblevm;
    }
    else
    {
        for(auto x : candidates)
        {
            for(int pm_index = 1; pm_index <=nPm; pm_index++)
            {
                for(int numa_index =1; numa_index <= nNuma; numa_index++)
                {
                        int leftovercpu1 = leftover[x.fi][x.se][pm_index][numa_index].cpu;
                        int leftovermemory1 = leftover[x.fi][x.se][pm_index][numa_index].memory;

                        int leftovercpu2 = leftover[x.fi][x.se][pm_index][partner[numa_index]].cpu;
                        int leftovermemory2 = leftover[x.fi][x.se][pm_index][partner[numa_index]].memory;

                        possiblevm += min(min(leftovercpu1/cpu , leftovermemory1/memory), min(leftovercpu2/cpu , leftovermemory2/memory));
                }
            }            
        }
        return possiblevm/2;
    }
}

//determine the max vm that we can allocate in resource/network/rack
// No consideration of partition.

int maxallocate(vector<VM> & vms)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;
    
    if(type==1)
    {
        for(int network_index = 1; network_index<=nNetwork; network_index++)
        {
            for(int rack_index = 1; rack_index <= nRack; rack_index++)
            {
                for(int pm_index = 1; pm_index <=nPm; pm_index++)
                {
                    for(int numa_index =1; numa_index <= nNuma; numa_index++)
                    {
                        int leftovercpu = leftover[network_index][rack_index][pm_index][numa_index].cpu;
                        int leftovermemory = leftover[network_index][rack_index][pm_index][numa_index].memory;
                        possiblevm += min(leftovercpu/cpu , leftovermemory/memory);
                    }
                }
            }
        }
        return possiblevm;
    }
    else
    {
        for(int network_index = 1; network_index<=nNetwork; network_index++)
        {
            for(int rack_index = 1; rack_index <= nRack; rack_index++)
            {
                for(int pm_index = 1; pm_index <=nPm; pm_index++)
                {
                    for(int numa_index =1; numa_index <= nNuma; numa_index++)
                    {
                        int leftovercpu1 = leftover[network_index][rack_index][pm_index][numa_index].cpu;
                        int leftovermemory1 = leftover[network_index][rack_index][pm_index][numa_index].memory;

                        int leftovercpu2 = leftover[network_index][rack_index][pm_index][partner[numa_index]].cpu;
                        int leftovermemory2 = leftover[network_index][rack_index][pm_index][partner[numa_index]].memory;

                        possiblevm += min(min(leftovercpu1/cpu , leftovermemory1/memory), min(leftovercpu2/cpu , leftovermemory2/memory));
                    }
                }
            }
        }
        return possiblevm/2;
    }
}

int maxallocate(vector<VM> & vms, int network)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;
    
    if(type==1)
    {
        
        for(int rack_index = 1; rack_index <= nRack; rack_index++)
        {
            for(int pm_index = 1; pm_index <=nPm; pm_index++)
            {
                for(int numa_index =1; numa_index <= nNuma; numa_index++)
                {
                    int leftovercpu = leftover[network][rack_index][pm_index][numa_index].cpu;
                    int leftovermemory = leftover[network][rack_index][pm_index][numa_index].memory;
                    possiblevm += min(leftovercpu/cpu , leftovermemory/memory);
                }
            }
        }
        return possiblevm;
    }
    else
    {
        for(int rack_index = 1; rack_index <= nRack; rack_index++)
        {
            for(int pm_index = 1; pm_index <=nPm; pm_index++)
            {
                for(int numa_index =1; numa_index <= nNuma; numa_index++)
                {
                    int leftovercpu1 = leftover[network][rack_index][pm_index][numa_index].cpu;
                    int leftovermemory1 = leftover[network][rack_index][pm_index][numa_index].memory;

                    int leftovercpu2 = leftover[network][rack_index][pm_index][partner[numa_index]].cpu;
                    int leftovermemory2 = leftover[network][rack_index][pm_index][partner[numa_index]].memory;

                    possiblevm += min(min(leftovercpu1/cpu , leftovermemory1/memory), min(leftovercpu2/cpu , leftovermemory2/memory));
                }
            }
        }
        return possiblevm/2;        
    }
}

int maxallocate(vector<VM> & vms, int network, int rack)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;
    
    if(type==1)
    {
        for(int pm_index = 1; pm_index <=nPm; pm_index++)
        {
            for(int numa_index =1; numa_index <= nNuma; numa_index++)
            {
                int leftovercpu = leftover[network][rack][pm_index][numa_index].cpu;
                int leftovermemory = leftover[network][rack][pm_index][numa_index].memory;

                possiblevm += min(leftovercpu/cpu , leftovermemory/memory);
            }
        }
        return possiblevm;
    }
    else
    {
        for(int pm_index = 1; pm_index <=nPm; pm_index++)
        {
            for(int numa_index =1; numa_index <= nNuma; numa_index++)
            {
                int leftovercpu1 = leftover[network][rack][pm_index][numa_index].cpu;
                int leftovermemory1 = leftover[network][rack][pm_index][numa_index].memory;

                int leftovercpu2 = leftover[network][rack][pm_index][partner[numa_index]].cpu;
                int leftovermemory2 = leftover[network][rack][pm_index][partner[numa_index]].memory;

                possiblevm += min(min(leftovercpu1/cpu , leftovermemory1/memory), min(leftovercpu2/cpu , leftovermemory2/memory));
            }
        }
        return possiblevm/2;

    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Calculate the most "unused" rack //////////////////////////////////////

vint find_optimal_network_order(vector<VM> & vms)
{
    vector<pii> candidate;

    vector<int> count(nNetwork+1);
    int maxval = 0;

    for(int i=1; i<=nNetwork; i++)
    {
        count[i] = 0;
        for(int j = 1 ; j<=nRack; j++)
        {
            count[i] += ( maxallocate(vms, i , j) );
        }
        candidate.pb({count[i], i});
        maxval = max(maxval, count[i]);
    }
    
    sort(candidate.rbegin(), candidate.rend());
    vint temp;
    for(auto x: candidate)
    {
        temp.pb(x.se);
    }
    return temp;
}


pii optimal_network_and_value(vector<VM> & vms)
{
    vector<pii> candidate;
    

    vector<int> count(nNetwork+1);
    int optimal_network = 1;
    int maxval = 0;

    for(int i=1; i<=nNetwork; i++)
    {
        count[i] = 0;
        for(int j = 1 ; j<=nRack; j++)
        {

            count[i] += ( maxallocate(vms, i , j) );
        }
        candidate.pb({count[i], i});
        if(count[i] > maxval)
        {
            optimal_network = i;
            maxval = count[i];
        }
    }
    return {optimal_network , maxval};
    
    
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// "update" algorithms ////////////////////////////////////////////
// This is where 
// 1. Storage, leftover, location of vm is updated

void update(VM  &vm0, int network, int rack, int pm, int numa)
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;
    int index = vm0.index;
    int pgindex = vm0.pg.index;
    int p = vm0.partition;


    Numanode & l1 = leftover[network][rack][pm][numa];
    set<int> & s1 = storage[network][rack][pm][numa];
    partition_storage[network][rack][pgindex][p] +=1;

    Numanode & l2 = leftover[network][rack][pm][partner[numa]];
    set<int> & s2 = storage[network][rack][pm][partner[numa]];


    if(type==1)
    {
        l1.cpu-=cpu; l1.memory -= memory; s1.insert(index); 
    }
    else
    {
        l1.cpu -= cpu; l1.memory -= memory; s1.insert(index);
        l2.cpu -= cpu; l2.memory -= memory; s2.insert(index);
    }
    vm[index].loc = {network, rack, pm, numa};
}


// 2.The answer vector is updated
void update_vector(vector<vint> & answer, int index, int network, int rack, int pm, int numa)
{
    int type;
    if(sz(answer[0])==4)type=1;
    else type =2;

    answer[index][0] = network;
    answer[index][1] = rack;
    answer[index][2] = pm;
    if(type==1)answer[index][3] = numa;
    else
    {
        answer[index][3] = numa;
        answer[index][4] = partner[numa];
    }
}


void update_multiple(vector<VM> & vms, int network, int rack, int pm, int numa, int start, int end)
{
    int n = end -start+1;
    int cpu = vms[start].info.cpu;
    int memory = vms[start].info.memory;
    int type = vms[start].info.numacount;
    int pgindex = vms[0].pg.index;


    Numanode & l1 = leftover[network][rack][pm][numa];
    set<int> & s1 = storage[network][rack][pm][numa];


    Numanode & l2 = leftover[network][rack][pm][partner[numa]];
    set<int> & s2 = storage[network][rack][pm][partner[numa]];

    


    if(type==1)
    {
        l1.cpu-=cpu*n; l1.memory -= memory*n; 
        for(int i = start; i<=end; i++)s1.insert(vms[i].index);

    }
    else
    {
        l1.cpu -= cpu*n; l1.memory -= memory*n; 
        l2.cpu -= cpu*n; l2.memory -= memory*n; 
        for(int i = start; i<=end; i++)s1.insert(vms[i].index);
        for(int i = start; i<=end; i++)s2.insert(vms[i].index);


    }
    for(int i = start; i<=end; i++)vm[vms[i].index].loc = {network, rack, pm, numa};
    for(int i = start; i<=end; i++)partition_storage[network][rack][pgindex][vms[i].partition]+=1;

}

void update_vector_multiple(vector<vint> & answer,  int network, int rack, int pm, int numa, int start, int end)
{
    int type;
    if(sz(answer[0])==4)type=1;
    else type =2;

    for(int index=start; index<=end; index++)
    {
        answer[index][0] = network;
        answer[index][1] = rack;
        answer[index][2] = pm;
        if(type==1)answer[index][3] = numa;
        else
        {
            answer[index][3] = numa;
            answer[index][4] = partner[numa];
        }
    }
}
//3.partners are updates
void update_partner(vector<VM> & vms)
{
    setval(2,1,4,3);
    int choice1 = maxallocate(vms);
    setval(3,4,1,2);
    int choice2 = maxallocate(vms);
    setval(4,3,2,1);
    int choice3 = maxallocate(vms);

    if(choice1 >= choice2 && choice1>=choice3)setval(2,1,4,3);
    else if(choice2>=choice1 && choice2>=choice3)setval(3,4,1,2);
}

void update_pgnetwork(int pgindex)
{
    bool found = false;
    for (int i=1; i<=nNetwork;i++)
    {
        for(int j=1; j<=nRack;j++)
        {
            for(int k=1; k<=nPm; k++)
            {
                for(int l=1; l<=nNuma; l++)
                {
                    for(auto x : storage[i][j][k][l])
                    {
                        if(vm[x].pg.index == pgindex)found = true;
                    }
                }
            }
        }
    }
    //update networkn
    if(found==false)
    {
        networknumofpg[pgindex]=0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////// where "answer", "storage","leftover" ,"loc" is updated //////////////////////////////////////
//////////////////////////////////// Should be called "once" ///////////////////////////////////////////////
// Already known that it is possible to assign.


//allocate vms into a single resource
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer)
{
        if(sz(vms)==0)return;

    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    
    int network_index = 1;
    int rack_index = 1;

   
    int fixedpointer = nNuma * (startingpm[vms[0].type]);
    int pointer =nNuma * (startingpm[vms[0].type]);
    int pm_index ; int numa_index;
    
    // the index of vms to assign location
    int toassign = 0;

    while(toassign < n)
    {
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;
                
        int maxallocatable = fullyallocate(vms[toassign],network_index, rack_index, pm_index, numa_index);
        int toallocate = min(maxallocatable, n-toassign);
        int start = toassign;
        int end = toassign + toallocate - 1;
        
        update_multiple(vms,network_index, rack_index, pm_index, numa_index,start, end);
        update_vector_multiple(answer, network_index, rack_index, pm_index, numa_index, start, end);
            //numa_index = partner[numa_index];
        toassign += toallocate;
        if(toassign==n)break;


        next(network_index,nNetwork);
        if(network_index==1 )pointer =  (pointer+1)%sz(permutation);
        if(pointer==fixedpointer && network_index==1)next(rack_index,nRack);

    }
}

//allocate vms into a network
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer, int network)
{
        if(sz(vms)==0)return;

    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int rack_index = 1;

    
    
    int fixedpointer = nNuma * (startingpm[vms[0].type]);
    int pointer =nNuma * (startingpm[vms[0].type]);
    int pm_index; int numa_index;
    int toassign = 0;

    while(toassign < n)
    {            
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;

        int maxallocatable = fullyallocate(vms[toassign],network, rack_index, pm_index, numa_index);
        int toallocate = min(maxallocatable, n-toassign);
        int start = toassign;
        int end = toassign + toallocate - 1;
        
        update_multiple(vms,network, rack_index, pm_index, numa_index,start, end);
        update_vector_multiple(answer, network, rack_index, pm_index, numa_index, start, end);
            //numa_index = partner[numa_index];
        toassign += toallocate;
        if(toassign==n)break;
        

        pointer= (pointer+1)%sz(permutation);
        if(pointer==fixedpointer)next(rack_index, nRack);
    }
}

// allocate vms into a single rack
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer, int network, int rack)
{
    if(sz(vms)==0)return;

    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;


    int fixedpointer = nNuma * (startingpm[vms[0].type]);
    int pointer =nNuma * (startingpm[vms[0].type]);
    int pm_index; int numa_index;

    int toassign = 0;

    while(toassign < n)
    {        
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;

        int maxallocatable = fullyallocate(vms[toassign],network, rack, pm_index, numa_index);
        int toallocate = min(maxallocatable, n-toassign);
        int start = toassign;
        int end = toassign + toallocate - 1;
        
        update_multiple(vms,network, rack, pm_index, numa_index,start, end);
        update_vector_multiple(answer, network, rack, pm_index, numa_index, start, end);
            //numa_index = partner[numa_index];
        toassign += toallocate;
        if(toassign==n)break;
        

        pointer = (pointer+1)%sz(permutation);
    }
}

// allocate one vm into a single rack
void allocate_one_with_no_constraint(VM vm0, vector<vector<int>> & answer, int i,int network, int rack)
{
    // info about vms
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;

    int fixedpointer = nNuma * (startingpm[vm0.type]);
    int pointer =nNuma * (startingpm[vm0.type]);
    int pm_index; int numa_index;
    int toassign = 0;

    while(toassign < 1)
    {
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;
                    
        if( canallocate(vm0,network, rack, pm_index, numa_index))
        {
            update(vm0,network, rack, pm_index, numa_index);
            update_vector(answer, i, network , rack, pm_index, numa_index);
                
            toassign ++;
        }

        pointer = (pointer+1)%sz(permutation);

    }
}


void allocate_among_candidates(vector<VM> & vms, vector<vector<int>> & answer, vpii & candidates)
{
    if(sz(vms)==0)return;
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int fixedpmpointer = nNuma * (startingpm[vms[0].type]);
    int pmpointer =nNuma * (startingpm[vms[0].type]);
    int pm_index ; int numa_index;

    // the index of vms to assign location
    int toassign = 0;

    int m =sz(candidates);
    int pointer = 0;


    while(toassign < n)
    {
        pm_index = numtoloc(permutation[pmpointer]).fi;
        numa_index = numtoloc(permutation[pmpointer]).se;
            
        int maxallocatable = fullyallocate(vms[toassign],candidates[pointer].fi,candidates[pointer].se, pm_index, numa_index);
        int toallocate = min(maxallocatable, n-toassign);
        int start = toassign;
        int end = toassign + toallocate - 1;
        
        update_multiple(vms,candidates[pointer].fi,candidates[pointer].se, pm_index, numa_index,start, end);
        update_vector_multiple(answer, candidates[pointer].fi,candidates[pointer].se, pm_index, numa_index, start, end);
            //numa_index = partner[numa_index];
        toassign += toallocate;
        if(toassign==n)break;
        

        // update the three indices

        pmpointer= (pmpointer+1)%sz(permutation);
        if(pmpointer==fixedpmpointer){pointer=(pointer+1)%m;}
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Network  : equal distribution //////////////////////////////
/////////////////////////////// Rack : from 1, FF //////////////////////////////////////////////

//choose the rack to place among all network, racks
//불가능하면 {0,0} return

pii choose_single_rack(vector<VM> & vms, int threshold)
{
    vector<int> v(nNetwork+1, 1000);
    for (int network=1 ; network<=nNetwork ; network++)
    {
        for(int rack = 1; rack<= nRack; rack++)
        {
            if(maxallocate(vms, network, rack)>=threshold)
            {
                v[network] = rack;
                break;
            }
        }
    }
    
    int minindex = 0;
    int minval = 1000;
    for(int  i=1; i<=nNetwork; i++)
    {
        if(v[i]<minval)
        {
            minval = v[i];
            minindex = i;
        }
    }
    if(minindex ==0)return {0,0};
    return {minindex, minval};
}

pii choose_single_rack_among_candidates(vector<VM> & vms, int threshold,  vpii & locations)
{
    // if empty, false
    if(sz(locations)==0)return {0,0};



        for (int i=0; i<sz(locations);i++)
        {
            pii x = locations[i];
            if(maxallocate(vms, x.fi, x.se)>=threshold)return x;
        }
    
    return  {0,0};
}

pair<int, pii> maximum_and_choose_single_rack(vector<VM> & vms, int threshold)
{
    int maximum = 0;
    vector<int> v(nNetwork+1, 1000);
    for (int network=1 ; network<=nNetwork ; network++)
    {
        for(int rack = 1; rack<= nRack; rack++)
        {
            int val = maxallocate(vms, network, rack);
            maximum += val;
            if(val>=threshold && v[network]==1000)v[network] = rack;
        }
    }
    
    int minindex = 0;
    int minval = 1000;
    for(int  i=1; i<=nNetwork; i++)
    {
        if(v[i]<minval)
        {
            minval = v[i];
            minindex = i;
        }
    }
    if(minindex ==0)return {maximum,{0,0}};
    return {maximum,{minindex, minval}};
}


int find_first_allocatable_network(vector<VM> & vms, int threshold)
{
    for(int i=1; i<=nNetwork;i++)
    {
        if(maxallocate(vms, i) >= threshold)return i;
    }
    return 0;

}

bool succeed(vector<VM> & vms, int network, int pgindex)
{
    int n = sz(vms);
    REP0(i,n)
    {
        bool updated = false;
        vpii candidates = possiblerack(pgindex, vms[i].partition, i);
        pii loc = choose_single_rack_among_candidates(vms, 1, candidates);
        if(loc!=make_pair(0,0))updated=true;
        if(updated==false)return false;
    }
    return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// main algorithm //////////////////////////////////////////

bool hard_rack_affinity_allocate(vector<VM> & vms, vector<vint> & answer, int n)
{
    // find where to locate
    pii loc = choose_single_rack(vms, n);
    //if there is no rack that can allocate
    if(loc == make_pair(0,0)){if(debugmode)print(1);return false;}
    // if possible
    allocate_with_no_constraint(vms, answer, loc.fi, loc.se);return true;
}

bool hard_network_affinity_undefined_allocate(vector<VM> & vms, vector<vint> & answer, int n, bool differentrack, int pgindex)
{   
    //They can be in the same rack in this case
    if(differentrack ==false) 
    {
        pii temp = optimal_network_and_value(vms);int network = temp.fi;int max_possible_allocation = temp.se;
        if(max_possible_allocation >= n )
            {
                networknumofpg[vms[0].pg.index] = network;
                allocate_with_no_constraint(vms, answer, network);
                return true;
            }
        else {if(debugmode)print(2);return false;}
    }
    //Every one of them should be in different rack, same net
    else
    {
        vint order = find_optimal_network_order(vms);

        for(auto i :order)
        {
            if(succeed(vms, i,pgindex))
            {
                networknumofpg[vms[0].pg.index] = i;
                REP0(i,n)
                {
                    vpii candidates = possiblerack(pgindex, vms[i].partition, i);
                    pii loc = choose_single_rack_among_candidates(vms, 1, candidates);
                    allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);
                    
                }
                return true;
            }
        }
        if(debugmode)print(3);return false;
            
    }

}

bool hard_network_affinity_defined_allocate(vector<VM> & vms, vector<vint> & answer, int n, int network, bool differentrack, int pgindex)
{
    //no partition constraint
    if(vms[0].partition==0)
    {
        int max_possible_allocation = maxallocate(vms, network);
        if(max_possible_allocation >= n){allocate_with_no_constraint(vms, answer, network);return true;}
        //impossible
        if(debugmode)print(4);return false;
        
    }
    // same partition constraint!=0
    else if(differentrack==false)
    {
        vpii candidates1 = findallrack(pgindex,vms[0].partition, network);
        vpii candidates2 =possiblerack(pgindex, vms[0].partition, network);

        
        int max_possible_allocation1 = maxallocate_candidates(vms, candidates1);
        if(max_possible_allocation1  >= n ){allocate_among_candidates(vms, answer, candidates1);return true;}
            
        
        
        int max_possible_allocation2 = maxallocate_candidates(vms, candidates2);
        if(max_possible_allocation2  >= n ){allocate_among_candidates(vms, answer, candidates2);return true;}
            
        //If everything fails
        if(debugmode)print(5);return false;
    }
    //Everything is in different rack, same given network
    else
    {
        REP0(i,n)
        {
            bool updatedanswer = false;
            
            vpii candidates = findallrack(pgindex, vms[i].partition, network);
            pii loc = choose_single_rack_among_candidates(vms, 1, candidates);
            if(loc!=make_pair(0,0))
            {
                updatedanswer = true;
                allocate_one_with_no_constraint(vms[i],  answer, i, loc.fi, loc.se);
            }
            
            if(updatedanswer==false)
            {
                vector<VM> temp = { vms[i]};
                vpii candidates = possiblerack(pgindex, vms[i].partition, network);
                pii loc = choose_single_rack_among_candidates(vms, 1, candidates);
                if(loc!=make_pair(0,0))
                {
                    updatedanswer=true;
                    allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);
                    
                }
            }
            if(updatedanswer==false){if(debugmode)print(6);return false;}
        }
        return true;
    }

}

bool no_affinity_allocate(vector<VM> & vms, vector<vint> & answer, int n, bool differentrack, int pgindex)
{
    // first. the case where there is no partition constraint
    if(vms[0].partition==0)
    {
        int max_possible_allocation =maxallocate(vms);
        if(max_possible_allocation < n){if(debugmode)print(7);return false;}
        else {allocate_with_no_constraint(vms, answer);return true;}    
    }
    // this is the case where they have same partition constraint
    else if(differentrack==false)
    {
        /*
        vpii candidates1 = findallrack(pgindex, vms[0].partition);
        vpii candidates2 = possiblerack(pgindex, vms[0].partition);
        
        int max_possible_allocation1 = maxallocate_candidates(vms, candidates1);
        if(max_possible_allocation1  >= n ){allocate_among_candidates(vms, answer, candidates1);return true;}
            
        int max_possible_allocation2 = maxallocate_candidates(vms, candidates2);
        if(max_possible_allocation2  >= n ){allocate_among_candidates(vms, answer, candidates2);return true;}
        if(debugmode)print(8);return false;
*/
        vpii candidates1 = findallrack(pgindex, vms[0].partition);
        vpii candidates2 = possiblerack(pgindex, vms[0].partition);
 
        vector<VM> vms1;
        vector<VM> vms2;
        vector<vint> answer1;
        vector<vint> answer2;
 
        int max_possible_allocation1 = maxallocate_candidates(vms, candidates1);
        for(int rep = 0; rep<min(n,max_possible_allocation1); rep++)
        {
            vms1.pb(vms[rep]);
            answer1.pb(answer[rep]);
        }
        for(int rep = min(n,max_possible_allocation1); rep<n; rep++)
        {
            vms2.pb(vms[rep]);
            answer2.pb(answer[rep]);
        }
        allocate_among_candidates(vms1, answer1, candidates1);
 
        int leftover = n - min(n,max_possible_allocation1);
 
        int max_possible_allocation2 = maxallocate_candidates(vms, candidates2);
        if(max_possible_allocation2  >=  leftover )
        {
            allocate_among_candidates(vms2, answer2, candidates2);
            REP0(i,  min(n,max_possible_allocation1))
            {
                answer[i] = answer1[i];
            }
            REP(i,  min(n,max_possible_allocation1), n-1)
            {
                answer[i] = answer2[i- min(n,max_possible_allocation1)];
            }
            return true;
        }
        if(debugmode)print(8);return false;

    }
    // This is the case where they all have different partition constraint
    else
    {
        REP0(i,n)
        {
            bool updatedanswer = false;
                
            vpii candidates = findallrack(pgindex, vms[i].partition);
            pii loc = choose_single_rack_among_candidates(vms, 1, candidates);
            if(loc != make_pair(0,0))
            {
                updatedanswer = true;
                allocate_one_with_no_constraint(vms[i],  answer, i, loc.fi, loc.se);
            }
            
            if(updatedanswer==false)
            {
                vector<VM> temp = { vms[i]};
                vpii candidates = possiblerack(pgindex, vms[i].partition);
                pii loc = choose_single_rack_among_candidates(vms,1, candidates);
                if(loc!=make_pair(0,0))
                {
                    updatedanswer=true;
                    allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);   
                }
            }
            
            if(updatedanswer==false){if(debugmode)print(9);return false;}
        }
        return true;
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VM_creation /////////////////////////////////////

// given vector of virtual machines, 
// 1. return whether it is possible to allocate
// 2. If so, return the allocation in answer vector
bool allocate(vector<VM> & vms, vector< vector<int> > & answer)
{
    //vms shares the same vm property and are in sam pg!!(might have different partition)

    int n = sz(vms);

    //information about the needed virtual machines
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    //information about the pg
    int raffinity = vms[0].pg.r_affinity;
    int naffinity = vms[0].pg.n_affinity;
    int pgindex= vms[0].pg.index;
    
    //lets just consider the hard constraints
    //hard constraints can be
    // 1. hard rack affinity
    // 2. hard network affinity
    // 3. hard rack anti affinity with partition(same partition cannot be in sam rack) 
    bool differentrack = false;
    if(n>=2 && vms[0].partition!= vms[1].partition)differentrack=true;



    // first the case where everything needs to be in the same rack
    // No partition conditions, called once
    if(raffinity ==2)return hard_rack_affinity_allocate(vms, answer, n);

    // The second case is when everything needs to be in the same network
    // There can be predefined network or not
    else if(naffinity == 2)
    {
        update_pgnetwork(pgindex); //remove can change the predefined
        int predefined_network = networknumofpg[vms[0].pg.index];


        if(predefined_network==0)return hard_network_affinity_undefined_allocate(vms,answer,n,differentrack,pgindex);
        else return hard_network_affinity_defined_allocate(vms, answer, n, predefined_network, differentrack, pgindex);
        
    }

    // The last case is where there is no condition on network, rack.
    // We only need to consider partition
    else return no_affinity_allocate(vms, answer,n, differentrack, pgindex);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VM_deletion /////////////////////////////////////////////////

vector<int> findVM(int index)
{
    // given the index of vm, find the network , rack, pm, numanode
    return vm[index].loc;
}

void deleteVM(int index)
{
    // delete the particular vm from storage
    // we need to keep track of storage, leftover, and family
    vint pos = findVM(index);
    if(vm[index].info.numacount==1)
    {
        storage[pos[0]][pos[1]][pos[2]][pos[3]].erase(index);
        partition_storage[pos[0]][pos[1]][vm[index].pg.index][vm[index].partition]--;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].memory += vm[index].info.memory;
        
    }
    else
    {
        storage[pos[0]][pos[1]][pos[2]][pos[3]].erase(index);
        storage[pos[0]][pos[1]][pos[2]][partner[pos[3]]].erase(index);
        partition_storage[pos[0]][pos[1]][vm[index].pg.index][vm[index].partition]--;


        leftover[pos[0]][pos[1]][pos[2]][pos[3]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].memory += vm[index].info.memory;

        leftover[pos[0]][pos[1]][pos[2]][partner[pos[3]]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][partner[pos[3]]].memory += vm[index].info.memory;
    }

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
    vmtotal += ncreate;
    
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
        newvm.type = typeofvm;

        temp.pb(newvm);
        vm.pb(newvm);
    }

    // check if we can allocate the new Vms and if so, print it out!
    // We use two functions each for numacount=1 and numacount=2
    //return whether it is possible or not
    if(nNuma ==4)update_partner(temp);
    bool possible;
    vector<vector<int>> answer;
    if (vmtype[typeofvm].numacount ==1 )
    {
        vector<int> v(4,0);
        answer.resize(ncreate, v);
    }
    else
    {
        vector<int> v(5,0);
        answer.resize(ncreate, v);

    }


    possible = allocate(temp, answer);

    if(possible)print2d(answer);
    else cout<<-1<<endl;
    
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


//make storage, leftover
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

// make partner for type 2 vm
void make_partner()
{
    partner.resize(1 + nNuma);
    vector<pii> count;
    for(int i=1 ; i<=nNuma; i++)
    {
        int temp = 0;
        for(int j =1; j<=sz(vmtype); j++)
        {
            if(vmtype[j].numacount==2)
            {
                temp += min(nodeinfo[i].cpu/vmtype[j].cpu , nodeinfo[i].memory/vmtype[j].memory);
            }
        }
        count.pb({temp, i});
    }
    sort(all(count));

    partner[count[0].se] = count[1].se;
    partner[count[1].se] = count[0].se;
    if(nNuma==4)
    {
        partner[count[2].se] = count[3].se;
        partner[count[3].se] = count[2].se;
    }
}

// read everything before request starts
void read_basic()
{
    //read in basic stuff like number of network, racks, PMs, NUMA nodes;
    cin>>nNetwork>>nRack>>nPm>>nNuma;

    nTotal =  nNetwork*nRack*nPm*nNuma;

    preprocess();

 
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
    make_partner();

}

// The main loop
int main()
{
    //efficient cin,cout
    ios::sync_with_stdio(0);
    //cin.tie(0);

    //read basic stuff
    read_basic();

    //read in request and handle (loop until termination(4))
    //request < 13500 , VMs < 235000
    int countr = 0;
    int request_type; cin>>request_type;
    while(request_type !=4 )
    {
        countr++;
        // test if the time constraint is the fundamental problem

        if(request_type==1)
        {
            //create placement group
            PG_creation();
        }
        else if(request_type == 2)
        {
            if(vmtotal > vmcutoff)
            {
                cout<<-1<<endl;
                break;
            }
            if(countr>timecutoff)
            {
                cout<<-1<<endl;
                break;
            }

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