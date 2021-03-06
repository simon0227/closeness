#include "Kazuya.h"

#define MAXLINE 50000
#define MAXTERM 100
#define THREADN 10
#define MAXDISTANCE (5)
#define phi (0.77351)
#define NMAP 128

#define EPSILON 1e-6

//#define MIXFRAMEWORK

/**
 * record the closeness cetrality of vertex id
 */
class vertex_centrality {
    public:
        long id;
        double closeness;
        vertex_centrality() { }
        vertex_centrality(double closeness_, int id_):id(id_), closeness(closeness_) { }

};


/**
 *  * record the average distance of vertex id
 *   */
class vertex_avgdist {
    public:
        int id;
        double avgdist;
        vertex_avgdist() { }
        vertex_avgdist(double avgdist_, int id_):id(id_), avgdist(avgdist_) { }

};

/**
 * record the vertex id and its degree
 */
class vertex_degree {
    public:
        int id;
        int degree;
        vertex_degree() { }
        vertex_degree(int degree_, int id_):id(id_), degree(degree_) { }

};


/*double abs(double t) {
    if (t < 0) return -t;
    return t;
}*/

inline bool operator< (const vertex_centrality &la, const vertex_centrality &lb) {
    if (abs(la.closeness - lb.closeness) > EPSILON) 
        return la.closeness > lb.closeness;
    return la.id < lb.id;

}

inline bool operator< (const vertex_avgdist &la, const vertex_avgdist &lb) {
    if (abs(la.avgdist - lb.avgdist) > EPSILON) 
        return la.avgdist < lb.avgdist;
    return la.id < lb.id;

}

inline bool operator< (const vertex_degree &la, const vertex_degree &lb) {
    if (la.degree != lb.degree) 
        return la.degree < lb.degree;
    return la.id < lb.id;

}

void ktopkrank(int k, vector<int> *person_graph, int num_person, int total_num_person, priority_queue <ValuePair> &ans);
void kcyclic_calculation(int qid, set<vertex_centrality>& answer, int k, vector<vertex_degree>& cc_vertices, int cc_size, 
        vector <int> * person_graph, int num_person, int total_num_person);
void kupdate_answer(set<vertex_centrality>& answer, int k, double centality, long vid);
void kbfs(int start, int* estimated_distances, vector <int> * person_graph, int total_num_person, int& estimated_radius);

void Kazuya::run(int k) {
    priority_queue<ValuePair> ans;

    int n = maxVertexId;
    vector <int> *vedge = new vector <int>[n + 1]; // edge list

    for (int i = 0; i < n; ++i)
        for (int j = graphSrc[i]; j < graphSrc[i + 1]; ++j)
            vedge[i].push_back(graphDst[j]);

    ktopkrank(k, vedge, n, n, ans);

    results.clear();
    while(!ans.empty()){
        ValuePair vp = ans.top();
        results.push_back(vp);
        ans.pop();
    }

    delete[] vedge;
}

vector<ValuePair> Kazuya::getResults() {
    return results;
}

inline unsigned int popcnt( unsigned int x ) {
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return x & 0x0000003f;
}

/**
 * Process each connected component one by one.
 * 
 * In each connected component, using cyclic select-test method.
 */
void ktopkrank(int k, vector<int> *person_graph, int num_person, int total_num_person, priority_queue <ValuePair> &ans) {
    int qid = 0;
    bool *visitedl = new bool[total_num_person + 1];
    vector<vertex_degree> cc_vertices;
    int cc_size = 0;
    queue <int> search_queue;
    memset(visitedl, 0, sizeof(bool) * (total_num_person + 1));
    set<vertex_centrality> answers;

    for (int idx = 0; idx < num_person; ++idx) {
        int start = idx;
        if (visitedl[start]) 
            continue;

        /* find one component using BFS. */
        search_queue.push(start);
        visitedl[start] = 1;
        cc_vertices.clear();
        cc_size = 0;
        while (!search_queue.empty()) {
            int cur = search_queue.front();
            search_queue.pop();
            cc_vertices.push_back(vertex_degree(person_graph[cur].size(), cur));
            ++cc_size;
            for (int nbr = 0; nbr < person_graph[cur].size(); ++nbr) {
                int nv = person_graph[cur][nbr];
                if (visitedl[nv])
                    continue;
                visitedl[nv] = 1;
                search_queue.push(nv);
            }
        }

        /* sort the vertices by the degree in ascending order.*/
        sort(cc_vertices.begin(), cc_vertices.end());

        /* execute K. Okamoto Algorithms */
        if(cc_size > 1)
            kcyclic_calculation(qid, answers, k, cc_vertices, cc_size, person_graph, num_person, total_num_person);
        else if(cc_size == 1) {
            kupdate_answer(answers, k, 0, cc_vertices[0].id);
        }
    }
    delete [] visitedl;

    /* output the answer. */
    char result[MAXLINE];
    char tmp[MAXTERM];
    //sprintf(result, "%d", qid);
    for (set <vertex_centrality>::iterator iter = answers.begin(); iter != answers.end(); ++iter) {
        //sprintf(tmp, " %ld %lf\n", (*iter).id, (*iter).closeness);
        //strcat(result, tmp);
        ans.push(ValuePair((*iter).id, (*iter).closeness));
    }

    //printf("%s\n", result);
}

/** 
 * Process procedure of delta_pfs.
 *
 */
void kprocess(int i, int *dist, int sumDist, set <vertex_centrality> &answer, int *father, int k, vector <int> *vedge, int sumN, int count, bool *prunable, int &total, int level, short& is_changed) {
    level++;
    double old_centrality;

    if (sumN > 1) old_centrality = (double)(sumN - 1) * (sumN - 1) / sumDist / (count - 1);
    else old_centrality = 0;

    int n = count;
    bool *v = new bool[n + 1];
    int *lambda = new int[n + 1];
    int beginT = clock();
    memmove(lambda, dist, sizeof(int) * (n + 1));
    int old_s = sumDist;
    for (int y = 0; y < vedge[i].size(); ++y) {
        int o = vedge[i][y];
        if (father[o] != i)
            continue;

        double centrality;
        if (sumN > 1) centrality = (double)(sumN - 1) * (sumN - 1) / (sumDist - sumN + 2) / (count - 1);
        else centrality = 0;
        if (answer.size() >= k && centrality < (answer.rbegin()->closeness - 1e-6) && prunable[o])
            continue;

        total++;

        sumDist += sumN - 2;
        dist[o] = dist[i] - 1;

        queue <int> list;
        memset(v, 0, sizeof(bool) * (n + 1));
        v[o] = 1;
        list.push(o);

        int cnt = 0;
        while (!list.empty()) {
            int cur = list.front();
            v[cur] = 1;
            list.pop();
            for (int t = 0; t < vedge[cur].size(); ++t) {
                int j = vedge[cur][t];
                cnt++;
                if (v[j])
                    continue;
                v[j] = 1;
                if (dist[cur] + 1 >= dist[j])
                    continue;
                list.push(j);
                sumDist -= dist[j] - (dist[cur] + 1);
                dist[j] = dist[cur] + 1;
            }
        }
        if (sumN > 1) centrality = (double)(sumN - 1) * (sumN - 1) / (sumDist) / (count - 1);
        else centrality = 0;
        answer.insert(vertex_centrality(centrality, o));
        if (answer.size() > k) {
            if(is_changed > 0 && answer.rbegin()->id != o) {
                is_changed *= -1;
            }
            answer.erase((*answer.rbegin()));
        }

        kprocess(o, dist, sumDist, answer, father, k, vedge, sumN, count, prunable, total, level, is_changed);

        memmove(dist, lambda, sizeof(int) * (n + 1));

        sumDist = old_s;
    }
    delete[] lambda;
    delete[] v;
}

/**
 * Calculate exact clossness centraliry value here.
 *
 * Using the method mentioned in "Efficient Top-k Closeness Centrality Search"
 * ICDE 2014
 */
void kdelta_pfs(set <vertex_centrality> &answer, int k, vector <vertex_degree> &vp, vector <int> *vedge, int total_num_person, int cc_size, int num_person, short& is_changed) {
    int n = total_num_person;

    bool *vpl = new bool[n + 1];
    bool *visitedl = new bool[n + 1];
    bool *v = new bool[n + 1];
    int *father = new int[n + 1];
    int *dist = new int[n + 1];
    bool *prunable = new bool[n + 1];
    queue <int> list;

    memset(vpl, 0, sizeof(bool) * (n + 1));
    memset(father, -1, sizeof(int) * (n + 1));
    memset(dist, 0, sizeof(int) * (n + 1));
    memset(prunable , 1, sizeof(bool) * (n + 1));

    for (int z = 0; z < vp.size(); ++z)
        vpl[vp[z].id] = 1;
    /*
     * find a schedule and vertice number of each component
     * */
    int total = 0;
    sort(vp.begin(), vp.end());	
    memset(visitedl, 0, sizeof(bool) * (n + 1));
    for (int z = 0; z < vp.size(); ++z) {
        int i = vp[z].id;
        if (visitedl[i]) continue;
        visitedl[i] = 1;
        int sumDist = 0, sumN = 0;		
        double centrality;

        memset(dist, 0, sizeof(int) * (n + 1));
        int maxd = 0;
        queue <int> schedule;
        schedule.push(i);

        /* calc dist map for vertex i */
        memset(v, 0, sizeof(bool) * (n + 1));
        v[i] = 1;
        list.push(i);
        while (!list.empty()) {
            int cur = list.front();
            v[cur] = 1;
            list.pop();
            sumDist += dist[cur];
            for (int t = 0; t < vedge[cur].size(); ++t) {
                int j = vedge[cur][t];
                if (v[j]) 
                    continue;
                v[j] = 1;
                list.push(j);
                dist[j] = dist[cur] + 1;
            }
        }

        /* determine the schedular from vertex i */
        list.push(i);
        while (!list.empty()) {
            int cur = list.front();
            visitedl[cur] = 1;
            list.pop();
            for (int t = 0; t < vedge[cur].size(); ++t) {
                int j = vedge[cur][t];
                if (visitedl[j] || !vpl[j])
                    continue;
                visitedl[j] = 1;
                list.push(j);
                father[j] = cur;
                prunable[cur] = 0;
            }
        }

        if (cc_size > 1) centrality = (double)(cc_size - 1) * (cc_size - 1) / (sumDist) / (num_person - 1);
        else centrality = 0;
        answer.insert(vertex_centrality(centrality, i));
        if (answer.size() > k) {
            if(is_changed > 0 && answer.rbegin()->id != i) {
                is_changed *= -1;
            }
            answer.erase((*answer.rbegin()));
        }

        /* from now, begin the delta-pfs */
        kprocess(i, dist, sumDist, answer, father, k, vedge, cc_size, num_person, prunable, total, 0, is_changed);
    }
    delete [] vpl;
    delete [] visitedl;
    delete [] v;
    delete [] father;
    delete [] dist;
    delete [] prunable;
}



/**
 * cyclic_calculation
 * 
 * Select some candidates by approximate method, calculate the exact centraliry value and update the top-k answers.
 * If the top-k answer set changed, do it again!
 */
void kcyclic_calculation(int qid, set<vertex_centrality>& answers, int k, vector<vertex_degree>& cc_vertices, int cc_size, vector<int> *person_graph, int num_person, int total_num_person) {
    int *exact2hop = new int[total_num_person + 1];

    int num_samples_1 = pow(cc_size, (double)2 / 3) * pow(log10(cc_size), (double)1 / 3);
    int alpha = 1;
    int num_samples = alpha * 100 * log10(cc_size);
    if(cc_size < num_samples)
        num_samples = cc_size;
    if (num_samples <= 1) num_samples = 2;
//    printf("cc_siez: %d  num_samples:%d n1=%d\n", cc_size, num_samples, num_samples_1);
    //num_samples = cc_size;
    //printf("%d\n", num_samples);
    int topke = 14 * k; // choose top 100*k of estimated closeness
    //if (total_num_person > 600000 && k < 5) topke = 1000;
    int topd = 0; // choose top 100 biggest of degree

    vector<vertex_avgdist> estimated_avgdist;
    vector<int> candidates;
    int* estimated_distances = new int[total_num_person+1];
    bool* is_in_candidates = new bool[total_num_person+1];
    int estimated_radius = -1; /**/
    memset(estimated_distances, 0, sizeof(int)*(total_num_person+1));
    memset(is_in_candidates, 0, sizeof(bool)*(total_num_person+1));

    /* using heuristic rules to select seeds and estimate the centrality! */
    for(int i = 0; i < num_samples; ++i) {
        int start = cc_vertices[i].id;
        int tmp_radius = -1;
        kbfs(start, estimated_distances, person_graph, total_num_person, tmp_radius);
        if(tmp_radius < estimated_radius || estimated_radius == -1)
            estimated_radius = tmp_radius;
    }

    //printf("estimated_radius: %d\n", estimated_radius);

    /*
    int n = total_num_person;
    long **sketch[2];	
    int *hashtable = new int[n + 1];
    int *total_estimated = new int[n + 1];
    memset(total_estimated, 0, sizeof(int) * (n + 1));
    sketch[0] = new long*[n + 1];
    sketch[1] = new long*[n + 1];
    int mask = NMAP - 1;
    int offset = popcnt(NMAP - 1);
    int bigmask = (1 << 16) - 1;
    int *hopin2 = new int[n + 1];
    memset(hopin2, -1, sizeof(int) * (n + 1));
    memset(exact2hop, 0, sizeof(int) * (n + 1));
    for (int iter = 0; iter < cc_size; ++iter) {
        int i = cc_vertices[iter].id;
        for (int j = 0; j < person_graph[i].size(); ++j) {
            int cur = person_graph[i][j];
            if (hopin2[cur] < iter) {
                hopin2[cur] = iter;
                exact2hop[i]++;
            }
            int length = person_graph[cur].size();
            for (int z = 0; z < length; ++z) {
                int u = person_graph[cur][z];
                if (hopin2[u] < iter) {
                    hopin2[u] = iter;
                    exact2hop[i]++;
                }

            }
        }
        estimated_distances[i] -= person_graph[i].size();
        estimated_distances[i] -= exact2hop[i];
    }

    delete [] hopin2;
    delete [] hashtable;
    for (int i = 0; i < 2; ++i) {
        delete [] sketch[i];
    }*/


    for(int i = 0; i < cc_size; ++i) {
        int vid = cc_vertices[i].id;
        double avgdist = estimated_distances[vid];
        estimated_avgdist.push_back(vertex_avgdist(avgdist, vid));
    }
    sort(estimated_avgdist.begin(), estimated_avgdist.end());

    /*int bound = 1 << 28;
    if (cc_size >= k) {
        bound = estimated_avgdist[k - 1].avgdist + (double)total_estimated[estimated_avgdist[0].id] * 1.65 * 0.068 * 0.4;
    }*/

    vector <vertex_degree> vp;
    /* calculate the candidate E */
    for(int i = 0; i < topke && i < estimated_avgdist.size(); ++i) {
        int id = estimated_avgdist[i].id;
        candidates.push_back(id);
        is_in_candidates[id] = 1;
        vp.push_back(vertex_degree(person_graph[id].size(), id));
    }
    /*for(int i = cc_vertices.size() - 1; i + topd >= cc_vertices.size() && i >=0; --i) {
        if(is_in_candidates[cc_vertices[i].id] == 0) {
            candidates.push_back(cc_vertices[i].id);
            vp.push_back(vertex_degree(cc_vertices[i].degree, cc_vertices[i].id));
        }
    }*/

    delete [] is_in_candidates;
    //estimated_distances = NULL;
    is_in_candidates = NULL;

    /* find the top-k in this connected component and merged into answer. */
    short is_changed = 0;
    //int inc = 50*k;

#ifdef MIXFRAMEWORK
    kdelta_pfs(answers, k, vp, person_graph, total_num_person, cc_size, num_person, is_changed);
#else
    for(int i = 0; i < cc_size; ++i) {
        int vid = cc_vertices[i].id;
        double avgdist = estimated_distances[vid];

    //    if (abs(avgdist < 1)) printf

        double centrality = (double)(cc_size - 1) / (avgdist / num_samples) / (num_person - 1);
        answers.insert(vertex_centrality(centrality, vid));
        if (answers.size() > k)
            answers.erase((*answers.rbegin()));

    }
#endif
    


    /*do{
        is_changed *= -1;
        is_changed++;
        vp.clear();
        for(int i = topke; i < topke+inc && i < estimated_avgdist.size(); ++i) {
            int id = estimated_avgdist[i].id;
            vp.push_back(vertex_degree(person_graph[id].size(), id));
        }
        topke += inc;
        if(vp.size() > 0) {
            kdelta_pfs(answers, k, vp, person_graph, total_num_person, cc_size, num_person, is_changed);
        }
    }while(is_changed < 0 && vp.size() > 0);*/
    delete [] exact2hop;
    estimated_avgdist.clear();
}

void kupdate_answer(set<vertex_centrality>& answer, int k, double centrality, long vid) {
    answer.insert(vertex_centrality(centrality, vid));
    if (answer.size() > k) 
        answer.erase((*answer.rbegin()));
}

void kbfs(int start, int* estimated_distances, vector <int> * person_graph, int total_num_person, int& estimated_radius) {
    int *dist = new int[total_num_person + 1];
    queue <int> search_queue;
    memset(dist, -1, sizeof(int) * (total_num_person + 1));

    /* compute shortest path using BFS. */
    search_queue.push(start);
    dist[start] = 0;

    while(!search_queue.empty()) {
        int cur = search_queue.front();
        search_queue.pop();
        estimated_distances[cur] += dist[cur];
        if(dist[cur] > estimated_radius)
            estimated_radius = dist[cur];

        for(int nbr = 0; nbr < person_graph[cur].size(); ++nbr) {
            int nv = person_graph[cur][nbr];
            if (dist[nv] != -1)
                continue;
            dist[nv] = dist[cur] + 1;
            search_queue.push(nv);
        }
    }
    delete [] dist;
}


