
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

///////////////////////////////////////////current status//////////////////////////////////////////////

//"no shuffling, meaning that we fill inside the rack in the same order"
//just don't think about max vm per pm constraint 


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// important variables ////////////////////////////////////////////

//이걸로 조절
bool debugmode = false;

int timecutoff = 15000;

int nNetwork, nRack, nPm, nNuma; 

int nTotal;


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
};
vector<VM> vm(1);   


// each component of a 4 dimensional vector is a set of integers
// storage[network][rack][pm][numa] : set of integers representing the Vms
vector<vector<vector<vector<  set<int>    >>>> storage;
vector<vector<vector<vector<     Numanode      >>>> leftover;


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

//get random num from 1~n
int random(int n)
{
    return (rand()%n)+1;
}
//next index considering circular relation
void next(int & a, int N)
{
    a = (a%N) + 1;
}


///////////////////////////////////////// generating permutations and determining order ///////////////////

vint permutation;   // 0~ nPm * nNuma-1 가 한번씩 나오는 순열

void preprocess()
{
    int pminr = nNuma * nPm;
    permutation.resize(pminr);
    REP0(i, pminr)permutation[i]=i;
}
void shuffle()
{
    random_shuffle(all(permutation));
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
    for(int pm_index = 1; pm_index <= nPm; pm_index++)
    {
        for(int numa_index =1 ; numa_index<=nNuma; numa_index++)
        {
            for( auto x : storage[network][rack][pm_index][numa_index])
            {
                if(vm[x].pg.index == pgindex && vm[x].partition!=p)
                {
                    return false;
                }
            }
        }
    }
    return true;
}


// determine if the rack contains vm with the particular partition p != 0
// O(#VM :  235000)
bool exist(int pgindex, int p, int network, int rack)
{
    for(int pm_index = 1; pm_index <= nPm; pm_index++)
    {
        for(int numa_index =1 ; numa_index<=nNuma; numa_index++)
        {
            for( auto x : storage[network][rack][pm_index][numa_index])
            {
                if(vm[x].pg.index == pgindex && vm[x].partition==p && p!=0)
                {
                    return true;

                }
            }
        }
    }
    return false;

}


//decide whether we can allocate vm0 in the given position
// consider every condition
// consider cpu,memory
// consider type
// consider partition
// consider network affinity
bool canallocate(VM vm0, int network, int rack, int pm, int numa)
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;
    int pgindex = vm0.pg.index;
    int npartition = vm0.pg.npartition;

    //network affinity condition
    if(networknumofpg[pgindex]!=0 && networknumofpg[pgindex]!= network)return false;
    //partition condition
    if(npartition!=0)
    {
        if(partitioncheck(network, rack, pgindex, vm0.partition)==false)return false;
    }
    //cpu, memory +(type) condition
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
    return racks;
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
    return racks;

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

// determine the max vm that we can allocate in resource considering partition
// the index of pg and the partition p that vms is sharing is given.
int maxallocate_constraint(vector<VM> & vms, int pgindex, int p)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;

    //get all possiblerack considering constraint
    vector<pii> candidate = possiblerack(pgindex, p);
    
    if(type==1)
    {
        for(auto x : candidate)
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
        for(auto x : candidate)
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

int maxallocate_constraint(vector<VM> & vms, int pgindex, int p, int network)
{
    int n =sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;

    int possiblevm = 0;
    
    //get all possiblerack considering constraint
    vector<pii> candidate = possiblerack(pgindex, p,network);
    
    if(type==1)
    {
        for(auto x : candidate)
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
        for(auto x : candidate)
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

int find_optimal_network(vector<VM> & vms)
{
    vector<int> candidate;

    vector<int> count(nNetwork+1);
    int maxval = 0;

    for(int i=1; i<=nNetwork; i++)
    {
        count[i] = 0;
        for(int j = 1 ; j<=nRack; j++)
        {

            count[i] += ( maxallocate(vms, i , j) );
        }
        maxval = max(maxval, count[i]);
    }
    
    REP1(i, nNetwork)
    {
        if(count[i]==maxval)candidate.pb(i);
       
    }

    int random_index = rand() % sz(candidate);
    return candidate[random_index];
    //return candidate[0];
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Calculate the most "unused" rack //////////////////////////////////////
// If multiple racks have same capacity, we randomly choose


// The candidates are "all racks"
pii find_optimal_rack(vector<VM> & vms)
{
    vector<pii> candidate;

    vector<vint> count(nNetwork+1);
    int maxval = -1;

    for(int i=1; i<=nNetwork; i++)
    {
        count[i].pb(-100);
        for(int j = 1 ; j<=nRack; j++)
        {

            count[i].pb( maxallocate(vms, i , j) );
            maxval = max(maxval, count[i][j]);
        }
    }
    REP1(i, nNetwork)
    {
        REP1(j, nRack)
        {
            if(count[i][j] ==maxval)candidate.pb({i,j});
        }
    }

    int random_index = rand() % sz(candidate);
    return candidate[random_index];
    //return candidate[0];
}

pii find_optimal_rack(vector<VM> & vms, int network)
{
    vector<pii> candidate;

    vector<int> count(nRack+1);
    int maxval = -1;

    
    for(int j = 1 ; j<=nRack; j++)
    {

        count[j] = maxallocate(vms, network , j) ;
        maxval = max(maxval, count[j]);
    }
    
    
    REP1(j, nRack)
    {
        if(count[j] ==maxval)candidate.pb({network,j});
    }
    

    int random_index = rand() % sz(candidate);
    return candidate[random_index];
    //return candidate[0];
}

// The candidates are explicitly given
pii find_optimal_rack_among_candidates(vector<VM> & vms, vpii & candidates)
{
    vector<pii> candidate;

    map<pii, int> count;
    int maxval = -1;

    for (auto x : candidates)
    {
        int temp = maxallocate(vms, x.fi, x.se);
        maxval = max(maxval, temp);
        count[x] = temp;
    }
    for (auto x : count)
    {
        if(x.se == maxval)
        {
            candidate.pb(x.fi);
        }
    }

    int random_index = rand() % sz(candidate);
    return candidate[random_index];
    //return candidate[0];
}


// If every rack doesn't satisfy the partition condition, we return {0,0}
pii find_optimal_rack_considering_partition(vector<VM> & vms, int pgindex, int p)
{
    vpii candidates =  possiblerack(pgindex, p);
    if(sz(candidates) == 0 )return {0,0};
    return find_optimal_rack_among_candidates(vms, candidates);
}

//Given the network condition,
// If every rack doesn't satisfy the partition condition, we return {0,0}
pii find_optimal_rack_considering_partition(vector<VM> & vms, int pgindex, int p, int network)
{
    vpii candidates =  possiblerack(pgindex, p, network);
    if(sz(candidates) == 0 )return {0,0};
    return find_optimal_rack_among_candidates(vms, candidates);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// "update" algorithms ////////////////////////////////////////////
// This is where 
// 1. Storage, leftover, location of vm is updated

void update(VM  vm0, int network, int rack, int pm, int numa)
{
    int cpu = vm0.info.cpu;
    int memory = vm0.info.memory;
    int type = vm0.info.numacount;
    int index = vm0.index;


    Numanode & l1 = leftover[network][rack][pm][numa];
    set<int> & s1 = storage[network][rack][pm][numa];

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

//4.update permutation corresponding to partner
void update_permutation()
{
    if(nNuma ==4)
    {
        if(partner[1]==2)
        {
            REP0(i, nPm)
            {
                permutation[i*4] = i*4 ;
                permutation[i*4+1] = i*4 +1;
                permutation[i*4+2] = i*4 + 2;
                permutation[i*4+3] = i*4 + 3;
            }

        }
        if(partner[1]==3)
        {
            REP0(i, nPm)
            {
                permutation[i*4] = i*4 ;
                permutation[i*4+1] = i*4 +2;
                permutation[i*4+2] = i*4 + 1;
                permutation[i*4+3] = i*4 + 3;
            }

        }
        if(partner[1]==4)
        {
            REP0(i, nPm)
            {
                permutation[i*4] = i*4 ;
                permutation[i*4+1] = i*4 +3;
                permutation[i*4+2] = i*4 + 1;
                permutation[i*4+3] = i*4 + 2;
            }

        }
        

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////// where "answer", "storage","leftover" ,"loc" is updated //////////////////////////////////////
//////////////////////////////////// Should be called "once" ///////////////////////////////////////////////
// Already known that it is possible to assign.



// Allocate only considering the cpu, memory, and type
// We don't think about the partition
//What we basically did is...
//For a single rack, distribute evenly in every numa, pm as possible
// Otherwise, fill in one rack fully then move,...

//allocate vms into a single resource
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer)
{
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    // random starting index 
    int network_index = random(nNetwork);
    int rack_index = random(nRack);
    //shuffle();
    int pointer =0;
    int pm_index ; int numa_index;
    
    // the index of vms to assign location
    int toassign = 0;

    while(toassign < n)
    {
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;
                
        while( canallocate(vms[toassign],network_index, rack_index, pm_index, numa_index))
        {
            update(vms[toassign],network_index, rack_index, pm_index, numa_index);
            update_vector(answer, toassign, network_index, rack_index, pm_index, numa_index);
            numa_index = partner[numa_index];
            toassign ++;
            if(toassign==n)break;
        }

        // update 
        pointer = (pointer+1)%sz(permutation);
        if(pointer==0)next(rack_index,nRack);
        if(pointer==0 && rack_index==1)next(network_index, nNetwork);
    }
}

//allocate vms into a network
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer, int network)
{
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    // random starting index 
    int rack_index = random(nRack);
    
    //shuffle();
    int pointer =0;
    int pm_index; int numa_index;
    // the index of vms to assign location
    int toassign = 0;

    while(toassign < n)
    {            
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;

        if( canallocate(vms[toassign],network, rack_index, pm_index, numa_index))
        {
            update(vms[toassign],network, rack_index, pm_index, numa_index);
            update_vector(answer, toassign, network, rack_index, pm_index, numa_index);
            toassign ++;
            if(toassign==n)break;
        }
        numa_index = partner[numa_index];
        if( type==1 && canallocate(vms[toassign],network, rack_index, pm_index, numa_index))
        {
            update(vms[toassign],network, rack_index, pm_index, numa_index);
            update_vector(answer, toassign, network, rack_index, pm_index, numa_index);
            toassign ++;
            if(toassign==n)break;
        }

        pointer= (pointer+1)%sz(permutation);
        if(pointer==0)next(rack_index, nRack);
    }
}

// allocate vms into a single rack
void allocate_with_no_constraint(vector<VM> & vms, vector<vector<int>> & answer, int network, int rack)
{
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    // random starting index 
    //shuffle();
    int pointer =0;
    int pm_index; int numa_index;

    int toassign = 0;

    while(toassign < n)
    {        
        pm_index = numtoloc(permutation[pointer]).fi;
        numa_index = numtoloc(permutation[pointer]).se;

        if( canallocate(vms[toassign],network, rack, pm_index, numa_index))
        {
            update(vms[toassign],network, rack, pm_index, numa_index);
            update_vector(answer, toassign, network, rack, pm_index, numa_index);
            toassign ++;
            if(toassign==n)break;
        }
        numa_index = partner[numa_index];
        if( type==1 && canallocate(vms[toassign],network, rack, pm_index, numa_index))
        {
            update(vms[toassign],network, rack, pm_index, numa_index);
            update_vector(answer, toassign, network, rack, pm_index, numa_index);
            toassign ++;
            if(toassign==n)break;
        }

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
    // random starting index of pm
    //shuffle();
    int pointer =0;
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





//allocate vms into a resource, but considering partition constraint
// We first calculate all possible candidates of racks then distribute, trying to put them in a single rack
void allocate_with_constraint(vector<VM> & vms, vector<vector<int>> & answer, int pgindex, int p)
{
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    // random starting index 

    //shuffle();
    int pmpointer =0;
    int pm_index ; int numa_index;

    // the index of vms to assign location
    int toassign = 0;

    vpii possibleracks = possiblerack(pgindex, p);
    int m =sz(possibleracks);
    int pointer = random(m)-1;

    while(toassign < n)
    {
        pm_index = numtoloc(permutation[pmpointer]).fi;
        numa_index = numtoloc(permutation[pmpointer]).se;

        // allocate in this position!!
        while( canallocate(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index))
        {
            update(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index);
            update_vector(answer, toassign, possibleracks[pointer].fi, possibleracks[pointer].se, pm_index, numa_index);
            numa_index = partner[numa_index];
            toassign ++;
            if(toassign==n)break;
        }
        // update the three indices
        pmpointer= (pmpointer+1)%sz(permutation);
        if(pmpointer==0){pointer=(pointer+1)%m;}
    }
}

void allocate_with_constraint(vector<VM> & vms, vector<vector<int>> & answer, int pgindex, int p, int network)
{
    // info about vms
    int n = sz(vms);
    int cpu = vms[0].info.cpu;
    int memory = vms[0].info.memory;
    int type = vms[0].info.numacount;
    // random starting index 

    //shuffle();
    int pmpointer =0;
    int pm_index ; int numa_index;

    // the index of vms to assign location
    int toassign = 0;

    vpii possibleracks = possiblerack(pgindex, p, network);
    int m =sz(possibleracks);
    int pointer = random(m)-1;

    while(toassign < n)
    {
        pm_index = numtoloc(permutation[pmpointer]).fi;
        numa_index = numtoloc(permutation[pmpointer]).se;
            
        // allocate in this position!!
        if( canallocate(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index))
        {
            update(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index);
            update_vector(answer, toassign, network, possibleracks[pointer].se,pm_index, numa_index );
            toassign ++;
            if(toassign==n)break;
        }
        numa_index = partner[numa_index];
        if( type == 1 && canallocate(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index))
        {
            update(vms[toassign],possibleracks[pointer].fi,possibleracks[pointer].se, pm_index, numa_index);
            update_vector(answer, toassign, network, possibleracks[pointer].se,pm_index, numa_index );
            toassign ++;
            if(toassign==n)break;
        }
        // update the three indices

        pmpointer= (pmpointer+1)%sz(permutation);
        if(pmpointer==0){pointer=(pointer+1)%m;}
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// main algorithm //////////////////////////////////////////

bool hard_rack_affinity_allocate(vector<VM> & vms, vector<vint> & answer, int n)
{
    // find where to locate
    pii loc = find_optimal_rack(vms);
    // check if it possible to allocate
    int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
    // if impossible, return false
    if(max_possible_allocation < n){if(debugmode)print(1);return false;}
    // if possible
    allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
    return true;
}

bool hard_network_affinity_undefined_allocate(vector<VM> & vms, vector<vint> & answer, int n, bool differentrack, int pgindex)
{   
    //They can be in the same rack in this case
    if(differentrack ==false) 
    {
        pii loc = find_optimal_rack(vms);
        int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
        if(max_possible_allocation >= n )
        {
            //update info
            networknumofpg[vms[0].pg.index] = loc.fi;
            allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
            return true;
        }
        else
        {
            int network = find_optimal_network(vms);
            int max_possible_allocation = maxallocate(vms, network);
            if(max_possible_allocation >=n)
            {
                networknumofpg[vms[0].pg.index] = network;
                allocate_with_no_constraint(vms, answer, network);
                return true;
            }
            else {if(debugmode)print(2);return false;}
        }
    }
    //Every one of them should be in different rack, same net
    else
    {
        int network = find_optimal_network(vms);
        REP0(i,n)
        {
            bool updatedanswer = false;
            vpii candidates = findallrack(pgindex, vms[i].partition,network);
            if(sz(candidates) > 0)
            {
                pii loc = find_optimal_rack_among_candidates(vms, candidates);
                if(maxallocate(vms, loc.fi, loc.se )>=1)
                {
                    updatedanswer = true;
                    allocate_one_with_no_constraint(vms[i],  answer, i, loc.fi, loc.se);
                }
            }
            if(updatedanswer==false)
            {
                vector<VM> temp = { vms[i]};
                pii loc = find_optimal_rack_considering_partition(temp, pgindex, vms[i].partition);
                if(loc!=make_pair(0,0))
                {
                    if (maxallocate(temp,loc.fi, loc.se )>=1)
                    {
                        updatedanswer=true;
                        allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);
                    }
                }
            }
            if(updatedanswer==false){if(debugmode)print(3);return false;}
        }
        return true;
    }

}

bool hard_network_affinity_defined_allocate(vector<VM> & vms, vector<vint> & answer, int n, int network, bool differentrack, int pgindex)
{
    //no partition constraint
    if(vms[0].partition==0)
    {
        pii loc = find_optimal_rack(vms, network);
        int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
        if(max_possible_allocation >= n )
        {
            allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
            return true;
        }
        max_possible_allocation = maxallocate(vms, loc.fi);
        if(max_possible_allocation >= n)
        {
            allocate_with_no_constraint(vms, answer, loc.fi);
            return true;
        }
        
        
        if(debugmode)print(4);
        return false;
        
    }
    // same partition constraint!=0
    else if(differentrack==false)
    {
        bool updatedanswer = false;

        // find all rack that already has that partition val
        vpii candidates = findallrack(pgindex, vms[0].partition, network);
        // Choose the optimal among the candidates
        if(sz(candidates) > 0)
        {
            pii loc = find_optimal_rack_among_candidates(vms, candidates);
            int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
            if(max_possible_allocation >= n)
            {
                allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
                updatedanswer = true;
            }
        }
        // If not, just try to choose from one rack that suffices the partition condition
        if(updatedanswer==false)
        {
            pii loc = find_optimal_rack_considering_partition(vms, pgindex, vms[0].partition, network);
            int max_possible_allocation = 0;
            if(loc!=make_pair(0,0))max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
            if(max_possible_allocation  >= n )
            {
                allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
                updatedanswer=true;
            }
        }
        // If that doesn't work, we need to use the whole network
        if(updatedanswer==false)
        {
            int max_possible_allocation = maxallocate_constraint(vms, pgindex, vms[0].partition, network);
            if(max_possible_allocation  >= n )
            {
                allocate_with_constraint(vms, answer, pgindex, vms[0].partition, network);
                updatedanswer=true;
            }
        }
        if(updatedanswer==false){if(debugmode)print(5);return false;}
        return true;
    }
    //Everything is in different rack, same given network
    else
    {
        REP0(i,n)
        {
            bool updatedanswer = false;
            vpii candidates = findallrack(pgindex, vms[i].partition, network);
            if(sz(candidates) > 0)
            {
                pii loc = find_optimal_rack_among_candidates(vms, candidates);
                if(maxallocate(vms, loc.fi, loc.se )>=1)
                {
                    updatedanswer = true;
                    allocate_one_with_no_constraint(vms[i],  answer, i, loc.fi, loc.se);
                }
            }
            if(updatedanswer==false)
            {
                vector<VM> temp = { vms[i]};
                pii loc = find_optimal_rack_considering_partition(temp, pgindex, vms[i].partition);
                if(loc!=make_pair(0,0))
                {
                    if (maxallocate(temp,loc.fi, loc.se )>=1)
                    {
                        updatedanswer=true;
                        allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);
                    }
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
        // We first try to put them in the same rack
        pii loc = find_optimal_rack(vms);
        int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
        if(max_possible_allocation  >= n ){allocate_with_no_constraint(vms, answer, loc.fi, loc.se);return true;}
        else
        {
            // now we use every resource to place 
            max_possible_allocation = maxallocate(vms);
            if(max_possible_allocation < n){if(debugmode)print(7);return false;}
            else {allocate_with_no_constraint(vms, answer);return true;}
        }
    }
    // this is the case where they have same partition constraint
    else if(differentrack==false)
    {
        bool updatedanswer = false;

        // find all rack that already has that partition val
        vpii candidates = findallrack(pgindex, vms[0].partition);
        // Choose the optimal among the candidates
        if(sz(candidates) > 0)
        {
            pii loc = find_optimal_rack_among_candidates(vms, candidates);
            int max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
            if(max_possible_allocation >= n)
            {
                allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
                updatedanswer = true;
            }
        }
        // If not, just try to choose from one rack that suffices the partition condition
        if(updatedanswer==false)
        {
            pii loc = find_optimal_rack_considering_partition(vms, pgindex, vms[0].partition);
            int max_possible_allocation = 0;
            if(loc!=make_pair(0,0))max_possible_allocation = maxallocate(vms, loc.fi, loc.se);
            if(max_possible_allocation  >= n )
            {
                allocate_with_no_constraint(vms, answer, loc.fi, loc.se);
                updatedanswer=true;
            }
        }
        // If that doesn't work, we need to use the whole resource
        if(updatedanswer==false)
        {
            int max_possible_allocation = maxallocate_constraint(vms, pgindex, vms[0].partition);
            if(max_possible_allocation  >= n )
            {
                allocate_with_constraint(vms, answer, pgindex, vms[0].partition);
                updatedanswer=true;
            }
        }
        if(updatedanswer==false){if(debugmode)print(8);return false;}
        return true;
    }
    // This is the case where they all have different partition constraint
    else
    {
        REP0(i,n)
        {
            bool updatedanswer = false;
                
            vpii candidates = findallrack(pgindex, vms[i].partition);
            if(sz(candidates) > 0)
            {
                pii loc = find_optimal_rack_among_candidates(vms, candidates);
                if(maxallocate(vms, loc.fi, loc.se )>=1)
                {
                    updatedanswer = true;
                    allocate_one_with_no_constraint(vms[i],  answer, i, loc.fi, loc.se);
                }
            }
            if(updatedanswer==false)
            {
                vector<VM> temp = { vms[i]};
                pii loc = find_optimal_rack_considering_partition(temp, pgindex, vms[i].partition);
                if(loc!=make_pair(0,0))
                {
                    if (maxallocate(temp,loc.fi, loc.se )>=1)
                    {
                        updatedanswer=true;
                        allocate_one_with_no_constraint(vms[i], answer, i, loc.fi,loc.se);
                    }
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
    int predefined_network = networknumofpg[vms[0].pg.index];
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
        if(predefined_network==0)return hard_network_affinity_undefined_allocate(vms,answer,n,differentrack,pgindex);
        else return hard_network_affinity_defined_allocate(vms, answer, n, predefined_network, differentrack, pgindex);
    }

    // The last case is where there is no condition on network, rack.
    // We only need to consider partition
    else return no_affinity_allocate(vms, answer,n, differentrack, pgindex);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VM_deletion /////////////////////////////////////////////////
//O(1)
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
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].cpu += vm[index].info.cpu;
        leftover[pos[0]][pos[1]][pos[2]][pos[3]].memory += vm[index].info.memory;
        
    }
    else
    {
        storage[pos[0]][pos[1]][pos[2]][pos[3]].erase(index);
        storage[pos[0]][pos[1]][pos[2]][partner[pos[3]]].erase(index);

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