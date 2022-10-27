#include <bits/stdc++.h>
using namespace std;
const int max_workpiece = 60;
const int max_machine = 20;
const int inf = 0x3f3f3f3f;
const int num_test_cases = 10;

struct workpiece
{
    // 基本信息存储
    int num_workpiece; //工件总数
    int num_machine;   //机器总数
    // 解（加工顺序）的存储
    int order_temp[max_workpiece];         //邻域中新生成的解，可能接受也可能拒绝
    int order_temp_current[max_workpiece]; //遍历邻域时存储的当前解，防止解的更新破坏了当前解
    int order_current[max_workpiece];      //当前解
    int order_best[max_workpiece];         //全局最优解（用于输出）
    // 加工时间的存储
    int time_work[max_workpiece][max_machine];           //每个工件在每台机器上的加工时间
    int time_finish_current[max_workpiece][max_machine]; //当前方案下加工的结束时间
    int time_finish_best[max_workpiece][max_machine];    //最优方案下加工的结束时间（用于输出）
    int total_time_current;                              //当前的加工总时间
    int total_time_best;                                 //最短的加工总时间（用于输出）
    // 蚁群算法参数的存储
    double pheromone[max_workpiece][max_workpiece]; //信息素量
    double eta[max_workpiece][max_workpiece];       //能见度eta
    // NEH算法参数的存储（辅助蚁群算法）
    int T[2][max_workpiece];            //每个工件在所有机器上的总加工时间，第一维存放时间，第二维存放对应的工件号（用于排序）
    int makespan;                       // NEH最优的加工总时间，用于蚁群算法中信息素的初始化
    int total_time_part[max_workpiece]; //前i工件最优的加工总时间，用于NEH中工件的插入
};

workpiece w[11];   //模拟退火算法结果存储
workpiece hc[11];  //爬山算法结果存储
workpiece aco[11]; //蚁群算法结果存储
workpiece neh[11]; // NEH算法结果存储

// 读入文件，并进行初始化
void input()
{
    //为了在运行时生成随机数，设置种子
    unsigned seed = time(0);
    srand(seed);
    ifstream fin;
    fin.open("data/flowshop-test-10-student.txt", ios::in);
    char tmp[100];
    for (int i = 0; i <= 10; i++)
    {
        fin.getline(tmp, sizeof(tmp));
        fin.getline(tmp, sizeof(tmp));
        fin >> w[i].num_workpiece >> w[i].num_machine;
        for (int j = 0; j < w[i].num_workpiece; j++)
        {
            w[i].order_current[j] = j;
            w[i].order_temp[j] = j;
            for (int k = 0; k < w[i].num_machine; k++)
            {
                w[i].time_finish_current[j][k] = 0;
                fin >> tmp >> w[i].time_work[j][k];
            }
        }
        //随机初始化工件加工顺序
        random_shuffle(w[i].order_current, w[i].order_current + w[i].num_workpiece - 1);
        w[i].total_time_current = inf;
        w[i].total_time_best = inf;
        aco[i] = hc[i] = w[i];
        //初始化蚁群算法特有参数
        for (int j = 0; j < aco[i].num_workpiece; j++)
        {
            aco[i].T[0][j] = 0;
            aco[i].T[1][j] = j;
            aco[i].total_time_part[j] = inf;
            for (int k = 0; k < aco[i].num_workpiece; k++)
            {
                aco[i].pheromone[j][k] = 1;
                aco[i].eta[j][k] = inf;
            }
        }
        aco[i].makespan = inf;
        neh[i] = aco[i];
        fin.getline(tmp, sizeof(tmp));
    }
}

// 通用模块

// 计算当前加工顺序下工件的加工时间
int calculate_time(workpiece w[11], int i, int num_workpiece)
{
    int cur = w[i].order_temp[0]; //当前被加工的工件
    w[i].time_finish_current[0][0] = w[i].time_work[cur][0];
    for (int k = 1; k < w[i].num_machine; k++)
        w[i].time_finish_current[0][k] = w[i].time_finish_current[0][k - 1] + w[i].time_work[cur][k];
    for (int j = 1; j < num_workpiece; j++)
    {
        int cur = w[i].order_temp[j];
        w[i].time_finish_current[j][0] = w[i].time_finish_current[j - 1][0] + w[i].time_work[cur][0];
        // 当前工件可以被加工：1.上一个在此机器加工的工件已经完成加工 2.该工件前一道工序已经完成
        for (int k = 1; k < w[i].num_machine; k++)
        {
            w[i].time_finish_current[j][k] = max(w[i].time_finish_current[j - 1][k], w[i].time_finish_current[j][k - 1]) + w[i].time_work[cur][k];
        }
    }
    return w[i].time_finish_current[num_workpiece - 1][w[i].num_machine - 1];
}
// 更新当前方案
void update(workpiece w[11], int i, int time_temp)
{
    w[i].total_time_current = time_temp;
    for (int j = 0; j < w[i].num_workpiece; j++)
    {
        w[i].order_current[j] = w[i].order_temp[j];
    }
}
// 记录最好的方案，用于后续对结果的输出（如果time_temp优于当前最优解则记录）
void record_if_best(workpiece w[11], int i, int time_temp)
{
    if (time_temp < w[i].total_time_best)
    {
        w[i].total_time_best = time_temp;
        for (int j = 0; j < w[i].num_workpiece; j++)
        {
            w[i].order_best[j] = w[i].order_temp[j];
        }
        for (int j = 0; j < w[i].num_workpiece; j++)
        {
            for (int k = 0; k < w[i].num_machine; k++)
            {
                w[i].time_finish_best[j][k] = w[i].time_finish_current[j][k];
            }
        }
    }
}
// 将最短时间及调度分配输出至文件，用于结果的保存和可视化
void output_to_file(workpiece w[11])
{
    ofstream ofs;  // 输出实现各个用例加工方案、每个工件在每台机器上的结束时间
    ofstream ofst; // 输出各个用例的最短完工时间
    ofs.open("data/result.txt", ios::out);
    ofst.open("data/minimum_time.txt", ios::out);
    for (int i = 0; i <= num_test_cases; i++)
    {
        ofs << i << endl;
        for (int j = 0; j < w[i].num_workpiece; j++)
        {
            ofs << w[i].order_best[j] << " ";
            for (int k = 0; k < w[i].num_machine; k++)
            {
                ofs << w[i].time_finish_best[j][k] << " ";
            }
            ofs << endl;
        }
    }
    for (int i = 0; i < 11; i++)
    {
        ofst << w[i].total_time_best << endl;
    }
    ofs.close();
    ofst.close();
}

// 优化模块

// 模拟退火算法
// 新解的生成——随机交换两个工件的加工顺序
void switch_order(workpiece w[11], int i)
{
    for (int j = 0; j < w[i].num_workpiece; j++)
    {
        w[i].order_temp[j] = w[i].order_current[j];
    }
    int x1 = rand() % w[i].num_workpiece;
    int x2 = 0;
    while (x1 == x2)
    {
        x2 = rand() % w[i].num_workpiece;
    }
    int temp = w[i].order_temp[x1];
    w[i].order_temp[x1] = w[i].order_temp[x2];
    w[i].order_temp[x2] = temp;
}
void SA()
{
    // 参数配置——初始温度
    double T[11] = {1000, 5000, 10000, 20000, 10000, 10000, 150000, 150000, 10000, 150000, 10000};
    // 参数配置——温度下限
    double eps[11] = {1e-7, 1e-7, 1e-7, 1e-7, 1e-7, 1e-7, 1e-7, 1e-6, 1e-7, 1e-7, 1e-7};
    // 参数配置——降温系数
    double dT[11] = {0.99, 0.95, 0.97, 0.99, 0.99, 0.99, 0.99, 0.998, 0.99, 0.997, 0.99};
    // 参数配置——同一温度下的迭代次数
    int t[11] = {100, 1000, 1000, 3000, 1000, 1000, 3000, 3000, 1000, 3000, 1000};
    cout << "===============Solution by Simulated Annealing===============" << endl;
    for (int i = 0; i <= num_test_cases; i++)
    {
        clock_t start, end; // 设计时钟，记录运行时间
        start = clock();
        while (T[i] > eps[i]) 
        {
            for (int k = 0; k < t[i]; k++)
            {
                // 生成新的方案，并计算时间
                switch_order(w, i);
                int time_temp = calculate_time(w, i, w[i].num_workpiece);
                record_if_best(w, i, time_temp); 
                //接受当前方案：1.比最小值小 2.以一定概率接受更差的状态 并记录此时的最小值和方案内容
                if (time_temp < w[i].total_time_current || exp((-time_temp + w[i].total_time_current) / T[i]) * RAND_MAX > rand())
                {
                    update(w, i, time_temp);
                }
            }
            T[i] *= dT[i]; //降温
        }
        end = clock();
        cout << "Minimum time of workpiece " << i << ": " << w[i].total_time_best << "     Run time: " << double(end - start) / CLOCKS_PER_SEC << "s" << endl;
    }
}

// 爬山算法
// 辅助爬山算法邻域的遍历——交换指定两个工件的加工顺序
void switch_order_HC(workpiece w[11], int i, int m, int n)
{
    for (int j = 0; j < w[i].num_workpiece; j++)
    {
        w[i].order_temp[j] = w[i].order_temp_current[j];
    }
    w[i].order_temp[m] = w[i].order_temp_current[n];
    w[i].order_temp[n] = w[i].order_temp_current[m];
}
// 邻域的遍历
bool traverse_neighborhood(workpiece hc[11], int i)
{
    // 存储当前解，用于对邻域的遍历
    for (int j = 0; j < hc[i].num_workpiece; j++)
    {
        hc[i].order_temp_current[j] = hc[i].order_current[j];
    }
    // 遍历登山算法所有邻域
    bool Update = false; // 记录是否对当前解进行更新
    for (int m = 0; m < hc[i].num_machine - 1; m++)
    {
        for (int n = m + 1; n < hc[i].num_machine; n++)
        {
            switch_order_HC(hc, i, m, n); // 交换当前解中m、n的顺序
            int time_temp = calculate_time(hc, i, hc[i].num_workpiece);
            if (time_temp < hc[i].total_time_current) // 如果交换后时间变短，则记录
            {
                // 虽然此处更新的是当前解order_current，但遍历时是根据order_temp_current交换解的顺序的，因此循环结束后，current内部存储的即为邻域的最优解
                update(hc, i, time_temp);
                Update = true;
            }
        }
    }
    return Update;
}
void HC()
{
    cout << "===============Solution by Hill Climbing===============" << endl;
    for (int i = 0; i <= num_test_cases; i++)
    {
        clock_t start, end;
        start = clock();
        for (int rep = 0; rep < 50000; rep++)
        {
            random_shuffle(hc[i].order_current, hc[i].order_current + hc[i].num_workpiece - 1);
            for (int k = 0; k < 1000; k++)
            {
                bool Update = traverse_neighborhood(hc, i);
                // 如果没有对当前解进行更新，则已登至山顶，跳出循环
                if (!Update)
                {
                    break;
                }
            }
            record_if_best(hc, i, hc[i].total_time_current);
        }
        end = clock();
        cout << "Minimum time of workpiece " << i << ": " << hc[i].total_time_best << "     Run time: " << double(end - start) / CLOCKS_PER_SEC << "s" << endl;
    }
}

// NEH算法（本实验中，用于与蚁群算法结合，辅助蚁群算法）
// 计算每个工件在所有机器上的总加工时间，用于算法步骤1
void calculate_time_workpiece(workpiece aco[11])
{
    for (int i = 0; i <= num_test_cases; i++)
    {
        for (int j = 0; j < aco[i].num_workpiece; j++)
        {
            for (int k = 0; k < aco[i].num_machine; k++)
            {
                aco[i].T[0][j] += aco[i].time_work[j][k];
            }
        }
    }
}
// 对每个工件在所有机器的总加工时间进行冒泡排序，用于算法步骤1
void bubble_sort(workpiece aco[11])
{
    for (int i = 0; i <= num_test_cases; i++)
    {
        for (int j = 0; j < aco[i].num_workpiece - 1; j++)
        {
            for (int k = 0; k < aco[i].num_workpiece - 1 - j; k++)
            {
                if (aco[i].T[0][k] < aco[i].T[0][k + 1])
                {
                    swap(aco[i].T[0][k], aco[i].T[0][k + 1]);
                    swap(aco[i].T[1][k], aco[i].T[1][k + 1]);
                }
            }
        }
    }
}
// 将工件插入指定位置，用于算法步骤3
void insert_order(workpiece aco[11], int i, int cur_workpiece, int place, int insert_workpiece_num)
{
    for (int j = cur_workpiece - 1; j >= place; j--)
    {
        aco[i].order_temp[j + 1] = aco[i].order_temp_current[j];
    }
    aco[i].order_temp[place] = insert_workpiece_num;
    for (int j = 0; j < place; j++)
    {
        aco[i].order_temp[j] = aco[i].order_temp_current[j];
    }
}
// 记录0~cur_workpiece的部分最短完工时间，用于算法步骤3
void record_if_best_part(workpiece w[11], int i, int time_temp, int cur_workpiece) // cur_workpiece：当前要插入的工件的下标
{
    if (time_temp < w[i].total_time_part[cur_workpiece]) // 如果更优，保存当前的完工时间、调度顺序等信息
    {
        w[i].total_time_part[cur_workpiece] = time_temp;
        for (int j = 0; j <= cur_workpiece; j++)
        {
            w[i].order_best[j] = w[i].order_temp[j];
        }
    }
}
// NEH算法主体
void NEH()
{
    cout << "===============Solution by NEH===============" << endl;
    // NEH步骤1：计算每个工件的总加工时间，并按照非增顺序排列
    calculate_time_workpiece(neh);
    bubble_sort(neh);
    for (int i = 0; i <= num_test_cases; i++)
    {
        clock_t start, end;
        start = clock();
        // NEH步骤2：取出步骤1排列好的前两个工件，选取时间较短的作为当前调度
        neh[i].order_temp[0] = neh[i].T[1][0];
        neh[i].order_temp[1] = neh[i].T[1][1];
        int time1 = calculate_time(neh, i, 2);
        swap(neh[i].order_temp[0], neh[i].order_temp[1]);
        int time2 = calculate_time(neh, i, 2);
        if (time1 < time2)
        {
            swap(neh[i].order_temp[0], neh[i].order_temp[1]);
        }
        // NEH步骤3：从第3个工件开始依次取出步骤1排列的好的工件，插入所有可能位置，将时间最短的排列作为当前调度
        for (int j = 2; j < neh[i].num_workpiece; j++)
        {
            int insert_num = neh[i].T[1][j];
            // 记录插入前的解
            for (int k = 0; k < j; k++)
            {
                neh[i].order_temp_current[k] = neh[i].order_temp[k];
            }
            // 进行所有可能的插入，记录时间最短的部分排列
            for (int k = 0; k <= j; k++)
            {
                insert_order(neh, i, j, k, insert_num);
                int time_temp = calculate_time(neh, i, j+1);
                record_if_best_part(neh, i, time_temp, j);
            }
            // 选取时间最短的部分排列作为新解，进行更新
            for (int k = 0; k <= j; k++)
            {
                neh[i].order_temp[k] = neh[i].order_best[k];
            }
        }
        // 得到工件的启发式最短调度时间makespan，可用于蚁群算法信息素的初始化
        aco[i].makespan = neh[i].total_time_part[neh[i].num_workpiece - 1];
        end = clock();
        cout << "Minimum time of workpiece " << i << ": " << aco[i].makespan << "     Run time: " << double(end - start) / CLOCKS_PER_SEC << "s" << endl;
    }
}

// 蚁群算法
// 与NEH的结合：插入型局部寻优的思想
bool insert_neighborhood(workpiece hc[11], int i)
{
    // 存储当前解，用于对邻域的遍历
    for (int j = 0; j < hc[i].num_workpiece; j++)
    {
        hc[i].order_temp_current[j] = hc[i].order_current[j];
    }
    bool Update = false; // 记录是否对当前解进行更新
    // 将第i（i=0~n-2）位置的工件依次插入第i+1~n-1位置，找到插入邻域中的最优解
    for (int m = 0; m < hc[i].num_machine - 1; m++)
    {
        for (int n = m + 1; n < hc[i].num_machine; n++)
        {
            for (int k = 0; k < hc[i].num_workpiece; k++)
            {
                // 将m位置的工件插入n
                if (k >= m && k < n)
                    hc[i].order_temp[k] = hc[i].order_temp_current[k + 1]; // m~n间的工件左移
                else if (k == n)
                    hc[i].order_temp[k] = hc[i].order_temp_current[m]; // n处插入m
                else
                    hc[i].order_temp[k] = hc[i].order_temp_current[k]; // 其余工件不变
            }
            int time_temp = calculate_time(hc, i, hc[i].num_workpiece);
            if (time_temp < hc[i].total_time_current)
            {
                update(hc, i, time_temp);
                Update = true;
            }
        }
    }
    return Update;
}
void ACO()
{
    // 参数设置
    int alpha = 1;
    int beta = 2; // alpha、beta用于概率的计算
    double rho = 0.3; // rho用于信息素的更新
    NEH();
    cout << "===============Solution by Ant Colony Optimization===============" << endl;
    for (int i = 0; i <= num_test_cases; i++)
    {
        clock_t start, end;
        start = clock();
        int num_ant = aco[i].num_workpiece * 1.5;
        // 二维数组，存放每个蚂蚁对应的排列顺序
        int ant_order[int(max_workpiece * 1.5)][max_workpiece];
        memset(ant_order, inf, sizeof(ant_order));
        // 信息素、能见度初始化
        for (int j = 0; j < aco[i].num_workpiece; j++)
        {
            for (int k = 0; k < aco[i].num_workpiece; k++)
            {
                aco[i].pheromone[j][k] = 1.0 / (aco[i].num_workpiece * aco[i].makespan);
                aco[i].eta[j][k] = neh[i].T[0][j];
            }
        }
        int Ncmax[11] = {200,500,5000,5000,3000,5000,5000,5000,5000,5000,5000};
        for (int rep = 0; rep < Ncmax[i]; rep++)
        {
            // 每个蚂蚁分别构造解
            for (int m = 0; m < num_ant; m++)
            {
                bool taboo[max_workpiece]; //记录已经被选择的城市
                memset(taboo, false, sizeof(taboo));
                double probability[max_workpiece]; // 记录被选择的概率
                // 进行每个工件的选择，共需要选择n（工件总数）次，每次可以选择的个数不超过n个
                for (int n = 0; n < aco[i].num_workpiece; n++)
                {
                    for (int k = 0; k < aco[i].num_workpiece; k++)
                    {
                        // 不能选择已经被选择过的工件
                        if (taboo[k] == true)
                            probability[k] = 0;
                        // 按照概率计算公式，计算选择当前工件的可能性
                        else
                            probability[k] = pow(aco[i].pheromone[n][k], alpha) * pow(aco[i].eta[n][k], beta);
                    }
                    // 对概率进行归一化，得到0~1之间的概率
                    int Sum = accumulate(probability, probability + aco[i].num_workpiece, 0);
                    for (int k = 0; k < aco[i].num_workpiece; k++)
                    {
                        probability[k] /= (Sum * 1.0);
                    }
                    // 随机生成一个0~1之间的概率
                    double pro = rand() * 1.0 / RAND_MAX; 
                    // 当前工件累计概率，用于工件的选取
                    double acc = 0;                       
                    int next;
                    for (int k = 0; k < aco[i].num_workpiece; k++)
                    {
                        if (!taboo[k])
                        {
                            acc += probability[k];
                            if (acc >= pro) 
                            {
                                next = k;
                                break;
                            }
                        }
                    }
                    // 更新已经选过的工件、更新蚂蚁选择的解
                    taboo[next] = true; 
                    ant_order[m][n] = next; 
                }
            }
            for (int m = 0; m < num_ant; m++)
            {
                // 对插入式寻优所需要的参数进行初始化
                for (int k = 0; k < aco[i].num_workpiece; k++)
                {
                    aco[i].order_current[k] = aco[i].order_temp[k] = ant_order[m][k]; 
                }
                int time_temp = aco[i].total_time_current = calculate_time(aco, i, aco[i].num_workpiece);
                // 进行插入式寻优，寻找插入邻域中的更优解，如果有更优的解，则进行更新
                if (insert_neighborhood(aco, i))
                {
                    time_temp = aco[i].total_time_current;
                    for (int k = 0; k < aco[i].num_workpiece; k++)
                    {
                        ant_order[m][k] = aco[i].order_current[k];
                    }
                }
                record_if_best(aco, i, time_temp);
                // 对信息素进行更新
                double delta_tau = 1.0 / time_temp;
                for (int n = 0; n < aco[i].num_workpiece; n++)
                {
                    int next = ant_order[m][n];
                    aco[i].pheromone[n][next] = (1 - rho) * aco[i].pheromone[n][next] + delta_tau;
                }
            }
        }
        end = clock();
        cout << "Minimum time of workpiece " << i << ": " << aco[i].total_time_best << "     Run time: " << double(end - start) / CLOCKS_PER_SEC << "s" << endl;
    }
}

int main()
{
    input();
    HC();
    SA();
    ACO();
    return 0;
}