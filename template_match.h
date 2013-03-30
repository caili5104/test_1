/*! @file
********************************************************************************
<PRE>
模块名       :Template Match
文件名       :template_match.h
相关文件     :
文件实现功能 :
作者         : hankscai
版本         : 1.0
--------------------------------------------------------------------------------
多线程安全性 :
异常时安全性 :
--------------------------------------------------------------------------------
备注         :
--------------------------------------------------------------------------------
修改记录     :
日 期        版本   修改人         修改内容
YYYY/MM/DD   1.0    <xxx>           创建
</PRE>
********************************************************************************

* 版权所有(c) YYYY, <xxx>, 保留所有权利

*******************************************************************************/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include "double_array_trie.h"
#include "TCSegFunc.h"

#define MAX_FRAGMENT_INDEX_LEVEL 10
#define IN
#define OUT
#define DUMMY
#define OPTIONAL
#define NOTE     //需要注意的代码
#define TODO	 //尚未实现的接口、类、算法等
#define UNDONE	 //已取消的接口、类、算法等
#define FOR_DBG	 //标记为调试方便而临时增加的代码
#define OK	 //仅用于调试的标记

#define RET_SUCCESS  1
#define RET_ERROR  0


//A * B C * D *         A = BEGINNING_TEMPLATE , BC = FORWARDING_TEMPLATE ,  D = ENDING_WITH_SLOT_TEMPLATE
enum Template_Index_Types_Based_On_Restrict_Fragment {BEGINNING_TEMPLATE = 1, FORWARDING_TEMPLATE, ENDING_WITH_SLOT_TEMPLATE, ENDING_TEMPLATE, WHOLE_TEMPLATE, HEAD_AND_SLOT_TEMPLATE};

using namespace std;

// =========================================================================
// 模板的数据结构，用于存储从模板文件中，读入的原始的模板的数据结构
/*
typedef struct
{
    string key_template;  //模板  例如 (^)([^\s]*?/ns) {0,1}到/[a-z]{1,2} ([^\s]*?/ns) {0,1}的/[a-z]{1,2} {0,1}火车票/[a-z]{1,2}$
    int weight;   //模板的权重
    int type;   //模板的类型
    int min_matched_term_num;  //模板的最小匹配词个数
    int max_matched_term_num;   //模板的最大的匹配词的个数
    int last_filling_term_index;  //最后一个“限定词”的位置

}Template_Property_Info;
*/

typedef struct
{
    vector<uint32_t>  	template_index_id;
    int 		term_num_in_corresponding_fragment;
    int 		position[10];
}Return_Template_Info;

typedef map<string,int> POS_Restrict;
typedef struct
{
    int 			type;
    string 			str_template;   //模板  例如 (^)([^\s]*?/ns) {0,1}到/[a-z]{1,2} ([^\s]*?/ns) {0,1}的/[a-z]{1,2} {0,1}火车票/[a-z]{1,2}$
    vector< vector<string> > 	key_template;
    uint64_t 			index_id;
    int 			min_matched_term_num;  //模板最小匹配的词数
    int 			max_matched_term_num;  //模板最大匹配的词数
    POS_Restrict 		pos_restrict; //词性的限制条件 如 ns -> 2 , 地名 必须 大于等于 2
    vector<Template_Index_Types_Based_On_Restrict_Fragment>  vec_restrict_fragment_type;
}Template_Property_Info;

// =========================================================================
//限定词的索引(加载完后的内存的存储形式)
typedef struct
{
    string 	      str_fragment;         //限定词   例如“到”
    int 	      term_num_in_corresponding_fragment;
    vector<uint32_t>  template_begin_index;
    vector<uint32_t>  template_forward_index;   // 除了该限定词，后面还有其他限定词的索引
    vector<uint32_t>  template_end_with_slot_index;
    vector<uint32_t>  template_end_index;    //   该限定词为最后一个限定词的模板的索引
    vector<uint32_t>  template_whole_match_index;
    vector<uint32_t>  template_head_and_slot_match_index;
}Fragment_Index;


// =========================================================================
//限定词的索引（加载中的中间转化格式）

typedef struct
{
    string	 str_fragment;
    int 	 term_num_in_corresponding_fragment;
    uint32_t 	 index;
    Template_Index_Types_Based_On_Restrict_Fragment fragment_index_type;
}Fragment_Corresponding_Template_Index_Tranform_Structure;


enum Query_Term_Match_Types_Based_On_Position {HEAD_MATCH = 1, MIDDLE_MATCH, END_MATCH, WHOLE_MATCH};

typedef struct
{
    string 	str_fragment;
    int 	term_num_in_corresponding_fragment;
    int 	begin_position;
    int 	end_position;
    int 	i_index_level;
    int 	index_template;
    Query_Term_Match_Types_Based_On_Position match_type;
}Fragment_Info;

typedef struct
{
    Fragment_Info key_infomation;
    Return_Template_Info return_info;
    vector<uint32_t>  template_forward_index;
    vector<uint32_t>  template_ending_index;
}Matched_Fragment_Candidate;



/*! @class
********************************************************************************
<PRE>
类名称   :
功能     :
异常类   :
--------------------------------------------------------------------------------
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/

class C_Template_Match
{
public:
    /////////////////////////////////////////////////////////////////////// 公用方法
    int Load_Template_File(IN const string& data_file);  //将模板文件导入到内存存储

    int Get_Match_Template_Info(IN const string& str_query, OUT vector<pair<uint64_t,string> >& g_match_template_info);

    int Get_Match_Template(IN const string& str_query, OUT vector<Template_Property_Info>& g_exact_match_template);

    int Show_Me(IN const string& str_input);  //用作测试

    //* A B * C *         vec_template: vector( C AB )    fragment_index_type: vector( ENDING_WITH_SLOT_TEMPLATE FORWARDING_TEMPLATE)
    int Turn_raw_template_into_processed_format(IN const string& str_template, OUT vector<vector<string> >& vec_template,
                                                OUT vector<Template_Index_Types_Based_On_Restrict_Fragment>& vec_fragment_index_type,IN bool (*fun_Is_Vec_Item_Slot)(IN const string& vec_item,OUT string& restrict_terms));

    C_Template_Match(IN const HANDLE& i_seg_handle);

    ~C_Template_Match(DUMMY);

private:
    /////////////////////////////////////////////////////////////////////////// 属性

    vector<Template_Property_Info>   m_Template_list;

    //vector<Template_Property_Info>   m_Template_list;  //所有模板的索引集合

    vector<Fragment_Index>   m_gIndex[MAX_FRAGMENT_INDEX_LEVEL];   //多级索引（最大支持10级）

    new_word::DATrie m_gTrie[MAX_FRAGMENT_INDEX_LEVEL];  //多级双数组trie树，和多级索引相对应

    HANDLE seg_handle;   //分词的句柄

    int m_iLevel;  //加载完后的多级数组的级数

private:
    //
    int Get_Rough_Template_From_Index_First_Step_Match(IN const string str_query_with_no_pos, OUT vector<uint32_t>& g_matched_templates);

    int Get_Exact_Match_Template_From_Feedback_Second_Step_Match(IN const string& str_query_with_pos,IN vector<uint32_t>& g_matched_templates,
                                                                       OUT vector<Template_Property_Info>& g_exact_match_template);

    //计算出模板中最后一个限定词的位置
   // int Judge_Ending_Index(IN const str_template, IN vector<vector<string> > vec_template, OUT vector<int>& fragment_index_type);

    //预测模板所能匹配的查询的最小词数和最大词数，这样有利于缩小正则表达式的匹配范围
    int Estimate_The_Min_Max_Match_Term_Num_Of_Template(IN OUT Template_Property_Info& template_property_info_temp);

    //从返回的可能模板索引中，精确匹配出完全匹配的模板，并返回模板参数


    //返回分词结果
    int Get_Seg_Result(IN const string str_query, OUT string& query_seg_no_pos, OUT string& query_seg_pos);

    int Turn_Vec_Template_Into_String(IN const vector<string>& vec_template,OUT string& str_key, OUT int& term_num_in_corresponding_fragment);

    int Get_Segment_Term_Num(IN const string& str_query);

    int Set_Corresponding_Index_Vec_Based_On_Fragment_Index_Types(IN const Template_Index_Types_Based_On_Restrict_Fragment& fragment_index_type, IN const int& index_id,
                                                                  OUT Fragment_Index& fragment_index_temp);

    int Push_Corresponding_Fragment_Index_Into_Vec_Based_On_Fragment_Position(IN OUT Fragment_Index& fragment_index_temp, IN const int& fragment_position);

    int Initialize_Fragment_Index_Temp(IN const string str_fragment,IN const int fragment_term_num,IN OUT Fragment_Index& fragment_index_temp);

    int Get_Matched_Fragment_Of_Each_Level_From_Raw_Query(IN const string str_query_with_no_pos,  OUT vector<Fragment_Info> (&vec_matched_fragment_on_i_th_level)[MAX_FRAGMENT_INDEX_LEVEL]);

    int Search_Candidate_Template_Basd_On_Matched_Fragment_Stack(IN const vector<Fragment_Info> (&vec_matched_fragment_on_i_th_level)[MAX_FRAGMENT_INDEX_LEVEL],OUT vector<uint32_t>& g_output_final);

    int Set_Fragment_Type_Based_On_Position_In_Query(IN const int& vec_size, IN OUT  Fragment_Info& fragment_info_temp);

};
