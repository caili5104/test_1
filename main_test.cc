#include "template_match.h"
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "TCSegFunc.h"
#include <time.h>
#include <sys/time.h>

DEFINE_string(template_file_name, "huoche_segment_sample.txt", "input template file name");

static const char segment_res_file[] = "./data";


vector<uint32_t> set_intersection_vec_template1(vector<uint32_t> input_1,vector<uint32_t> input_2)
{
    vector<uint32_t> g_temp;
    g_temp.clear();
    set_intersection(input_1.begin(), input_1.end(),
    input_2.begin(), input_2.end(),
     back_inserter(g_temp));
    return g_temp;
}
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, false);
    cout << "test template_file_name:" << FLAGS_template_file_name << endl;
    LOG(ERROR) << "BEGIN";
    time_t start_t;
    time_t end_t;


    start_t= time((time_t*)NULL);


    HANDLE seg_handle;

    //string query="我的祖国是中国";

    if ( TCInitSeg(segment_res_file)) {
		int SEG_MODE = TC_CN|TC_POS|TC_S2D|TC_U2L|TC_USR|TC_CLS;
		seg_handle = TCCreateSegHandle(SEG_MODE);
	} else{
		seg_handle = NULL;
	}

    C_Template_Match *p_template_match = new C_Template_Match(seg_handle);

    p_template_match->Show_Me("I am  here");

    p_template_match->Load_Template_File(FLAGS_template_file_name);

    LOG(ERROR) << "Loading successfully";

    end_t= time((time_t*)NULL);

    LOG(ERROR) << "Time used" <<(end_t - start_t);


/*
      new_word::DATrie da_trie;


     vector<const char*>  words_vec;
    words_vec.push_back("abcd");
    words_vec.push_back("dsdsad");


        sort(words_vec.begin(),words_vec.end());
        vector<int>  vec_int;
        vec_int.push_back(0);
        vec_int.push_back(1);
        int index[] = {2,1};


            if(da_trie.Build(words_vec.size(), &words_vec[0], 0 , &vec_int[0]))
            {
                    int result[16];
                    size_t num = da_trie.CommonPrefixSearch("abcde", result, 16);
                    LOG(ERROR)<<num;
                    for (size_t j = 0; j < num; ++j)
                    {
                        LOG(ERROR)<<result[j];
                        LOG(ERROR)<<words_vec[result[j]];
                    }



            }

*/
        /*
        for (size_t i = 0; i < words_vec.size(); ++i)
        {
            cout<<da_trie.ExactMatchSearch(words_vec[i])<<endl;
        }
        */

     struct timeval tvafter,tvpre;
    struct timezone tz;
    int sum = 0;




    vector<string>  a_test;


    a_test.push_back("北京到上海的火车");
    a_test.push_back("关于的中国的文章");
    a_test.push_back("大庆到长春的火车票2624次");
     a_test.push_back("1月25号安徽芜湖到上海客车时间表");



     start_t= time((time_t*)NULL);


    for(int j =0; j< a_test.size();j++)

{


    string query=a_test[j];
    cout<<query<<endl;
    vector<Template_Property_Info> g_exact_match_template;

    gettimeofday (&tvpre , &tz);
    p_template_match->Get_Match_Template(query, g_exact_match_template);
     gettimeofday (&tvafter , &tz);
    printf("sum=%d usec%d\n",sum, (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);

    for(size_t i =0; i< g_exact_match_template.size();i++)
    {
        cout<<i<<endl;
        cout<<g_exact_match_template[i].str_template<<endl;;
    }


    LOG(ERROR) << "SIZE" <<g_exact_match_template.size();

}




    end_t= time((time_t*)NULL);

     TCCloseSegHandle(seg_handle);                // 关闭句柄
      TCUnInitSeg();                              // 卸载资源

    return 1;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               