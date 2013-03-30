/*! @file
********************************************************************************
<PRE>
ģ����       :Template Match
�ļ���       :template_match.h
����ļ�     :
�ļ�ʵ�ֹ��� :
����         : hankscai
�汾         : 1.0
--------------------------------------------------------------------------------
���̰߳�ȫ�� :
�쳣ʱ��ȫ�� :
--------------------------------------------------------------------------------
��ע         :
--------------------------------------------------------------------------------
�޸ļ�¼     :
�� ��        �汾   �޸���         �޸�����
YYYY/MM/DD   1.0    <xxx>           ����
</PRE>
********************************************************************************

* ��Ȩ����(c) YYYY, <xxx>, ��������Ȩ��

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
#define NOTE     //��Ҫע��Ĵ���
#define TODO	 //��δʵ�ֵĽӿڡ��ࡢ�㷨��
#define UNDONE	 //��ȡ���Ľӿڡ��ࡢ�㷨��
#define FOR_DBG	 //���Ϊ���Է������ʱ���ӵĴ���
#define OK	 //�����ڵ��Եı��

#define RET_SUCCESS  1
#define RET_ERROR  0


//A * B C * D *         A = BEGINNING_TEMPLATE , BC = FORWARDING_TEMPLATE ,  D = ENDING_WITH_SLOT_TEMPLATE
enum Template_Index_Types_Based_On_Restrict_Fragment {BEGINNING_TEMPLATE = 1, FORWARDING_TEMPLATE, ENDING_WITH_SLOT_TEMPLATE, ENDING_TEMPLATE, WHOLE_TEMPLATE, HEAD_AND_SLOT_TEMPLATE};

using namespace std;

// =========================================================================
// ģ������ݽṹ�����ڴ洢��ģ���ļ��У������ԭʼ��ģ������ݽṹ
/*
typedef struct
{
    string key_template;  //ģ��  ���� (^)([^\s]*?/ns) {0,1}��/[a-z]{1,2} ([^\s]*?/ns) {0,1}��/[a-z]{1,2} {0,1}��Ʊ/[a-z]{1,2}$
    int weight;   //ģ���Ȩ��
    int type;   //ģ�������
    int min_matched_term_num;  //ģ�����Сƥ��ʸ���
    int max_matched_term_num;   //ģ�������ƥ��ʵĸ���
    int last_filling_term_index;  //���һ�����޶��ʡ���λ��

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
    string 			str_template;   //ģ��  ���� (^)([^\s]*?/ns) {0,1}��/[a-z]{1,2} ([^\s]*?/ns) {0,1}��/[a-z]{1,2} {0,1}��Ʊ/[a-z]{1,2}$
    vector< vector<string> > 	key_template;
    uint64_t 			index_id;
    int 			min_matched_term_num;  //ģ����Сƥ��Ĵ���
    int 			max_matched_term_num;  //ģ�����ƥ��Ĵ���
    POS_Restrict 		pos_restrict; //���Ե��������� �� ns -> 2 , ���� ���� ���ڵ��� 2
    vector<Template_Index_Types_Based_On_Restrict_Fragment>  vec_restrict_fragment_type;
}Template_Property_Info;

// =========================================================================
//�޶��ʵ�����(���������ڴ�Ĵ洢��ʽ)
typedef struct
{
    string 	      str_fragment;         //�޶���   ���硰����
    int 	      term_num_in_corresponding_fragment;
    vector<uint32_t>  template_begin_index;
    vector<uint32_t>  template_forward_index;   // ���˸��޶��ʣ����滹�������޶��ʵ�����
    vector<uint32_t>  template_end_with_slot_index;
    vector<uint32_t>  template_end_index;    //   ���޶���Ϊ���һ���޶��ʵ�ģ�������
    vector<uint32_t>  template_whole_match_index;
    vector<uint32_t>  template_head_and_slot_match_index;
}Fragment_Index;


// =========================================================================
//�޶��ʵ������������е��м�ת����ʽ��

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
������   :
����     :
�쳣��   :
--------------------------------------------------------------------------------
��ע     :
�����÷� :
--------------------------------------------------------------------------------
����     : <xxx>
</PRE>
*******************************************************************************/

class C_Template_Match
{
public:
    /////////////////////////////////////////////////////////////////////// ���÷���
    int Load_Template_File(IN const string& data_file);  //��ģ���ļ����뵽�ڴ�洢

    int Get_Match_Template_Info(IN const string& str_query, OUT vector<pair<uint64_t,string> >& g_match_template_info);

    int Get_Match_Template(IN const string& str_query, OUT vector<Template_Property_Info>& g_exact_match_template);

    int Show_Me(IN const string& str_input);  //��������

    //* A B * C *         vec_template: vector( C AB )    fragment_index_type: vector( ENDING_WITH_SLOT_TEMPLATE FORWARDING_TEMPLATE)
    int Turn_raw_template_into_processed_format(IN const string& str_template, OUT vector<vector<string> >& vec_template,
                                                OUT vector<Template_Index_Types_Based_On_Restrict_Fragment>& vec_fragment_index_type,IN bool (*fun_Is_Vec_Item_Slot)(IN const string& vec_item,OUT string& restrict_terms));

    C_Template_Match(IN const HANDLE& i_seg_handle);

    ~C_Template_Match(DUMMY);

private:
    /////////////////////////////////////////////////////////////////////////// ����

    vector<Template_Property_Info>   m_Template_list;

    //vector<Template_Property_Info>   m_Template_list;  //����ģ�����������

    vector<Fragment_Index>   m_gIndex[MAX_FRAGMENT_INDEX_LEVEL];   //�༶���������֧��10����

    new_word::DATrie m_gTrie[MAX_FRAGMENT_INDEX_LEVEL];  //�༶˫����trie�����Ͷ༶�������Ӧ

    HANDLE seg_handle;   //�ִʵľ��

    int m_iLevel;  //�������Ķ༶����ļ���

private:
    //
    int Get_Rough_Template_From_Index_First_Step_Match(IN const string str_query_with_no_pos, OUT vector<uint32_t>& g_matched_templates);

    int Get_Exact_Match_Template_From_Feedback_Second_Step_Match(IN const string& str_query_with_pos,IN vector<uint32_t>& g_matched_templates,
                                                                       OUT vector<Template_Property_Info>& g_exact_match_template);

    //�����ģ�������һ���޶��ʵ�λ��
   // int Judge_Ending_Index(IN const str_template, IN vector<vector<string> > vec_template, OUT vector<int>& fragment_index_type);

    //Ԥ��ģ������ƥ��Ĳ�ѯ����С��������������������������С������ʽ��ƥ�䷶Χ
    int Estimate_The_Min_Max_Match_Term_Num_Of_Template(IN OUT Template_Property_Info& template_property_info_temp);

    //�ӷ��صĿ���ģ�������У���ȷƥ�����ȫƥ���ģ�壬������ģ�����


    //���طִʽ��
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
