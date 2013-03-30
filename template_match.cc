#include "template_match.h"
#include "common/base/string/algorithm.h"
#include "common/text/regex/regex.h"
#include "common/base/string/string_number.h"
#include "glog/logging.h"
#include "double_array_trie.h"
#include <sys/time.h>

#define GET_ARRAY_LEN(array,len){len = (sizeof(array) / sizeof(array[0]));}

using namespace std;


C_Template_Match::C_Template_Match(IN const HANDLE& i_seg_handle)
{




// =========================================================================
// 初始化默认的多级双数组的级数  10级

	m_iLevel = MAX_FRAGMENT_INDEX_LEVEL;

}

C_Template_Match::~C_Template_Match(DUMMY)
{
// =========================================================================
//  释放资源


}

// =========================================================================
// 按Elem_Index中key的主键进行排序

bool Comp_Fragment(const Fragment_Corresponding_Template_Index_Tranform_Structure& p1, const Fragment_Corresponding_Template_Index_Tranform_Structure& p2)
{
    return p1.str_fragment < p2.str_fragment;
}

//  (^)([^\s]*?/ns)  判断这个是不是slot
bool Is_Vec_Item_Slot(IN const string& vec_item,OUT string& restrict_terms)
{
    Regex re_judge_slot("(.*?)}(.*?)/(.*?)");
    string str_token1;
    string str_token2;
    re_judge_slot.FullMatch(vec_item, &str_token1,&str_token2);
    if(str_token2.size()==0)
    {

        return true;
    }
    else
    {
        restrict_terms =  str_token2;
        return false;
    }
}




bool Is_Vec_Item_Slot_Old(IN const string& vec_item,OUT string& restrict_terms)
{

    if(vec_item == "*")
    {
        return true;
    }
    else
    {
        restrict_terms =  vec_item;
        return false;
    }

}

int C_Template_Match::Initialize_Fragment_Index_Temp(IN const string str_fragment,IN const int fragment_term_num,IN OUT Fragment_Index& fragment_index_temp)
 {

    fragment_index_temp.str_fragment = str_fragment;
    fragment_index_temp.term_num_in_corresponding_fragment = fragment_term_num;

    fragment_index_temp.template_begin_index.clear();
    fragment_index_temp.template_forward_index.clear();
    fragment_index_temp.template_end_with_slot_index.clear();
    fragment_index_temp.template_end_index.clear();
    fragment_index_temp.template_whole_match_index.clear();
    fragment_index_temp.template_head_and_slot_match_index.clear();
     return RET_SUCCESS;
 }

int C_Template_Match::Push_Corresponding_Fragment_Index_Into_Vec_Based_On_Fragment_Position(IN OUT Fragment_Index& fragment_index_temp, IN const int& fragment_position)
{
    sort(fragment_index_temp.template_begin_index.begin(),fragment_index_temp.template_begin_index.end());
    sort(fragment_index_temp.template_forward_index.begin(),fragment_index_temp.template_forward_index.end());
    sort(fragment_index_temp.template_end_with_slot_index.begin(),fragment_index_temp.template_end_with_slot_index.end());
    sort(fragment_index_temp.template_end_index.begin(),fragment_index_temp.template_end_index.end());
    sort(fragment_index_temp.template_whole_match_index.begin(),fragment_index_temp.template_whole_match_index.end());
    sort(fragment_index_temp.template_head_and_slot_match_index.begin(),fragment_index_temp.template_head_and_slot_match_index.end());
    m_gIndex[fragment_position].push_back(fragment_index_temp);

    return RET_SUCCESS;
}

int C_Template_Match::Set_Corresponding_Index_Vec_Based_On_Fragment_Index_Types(IN const Template_Index_Types_Based_On_Restrict_Fragment& fragment_index_type, IN const int& index_id,
                                                                                OUT Fragment_Index& fragment_index_temp)
{
     switch(fragment_index_type)
    {
        case BEGINNING_TEMPLATE:
        {
            fragment_index_temp.template_begin_index.push_back(index_id);
            break;
        }
        case FORWARDING_TEMPLATE:
        {
            fragment_index_temp.template_forward_index.push_back(index_id);
            break;
        }
        case ENDING_WITH_SLOT_TEMPLATE:
        {
            fragment_index_temp.template_end_with_slot_index.push_back(index_id);
            break;
        }
        case ENDING_TEMPLATE:
        {
            fragment_index_temp.template_end_index.push_back(index_id);
            break;
        }
        case WHOLE_TEMPLATE:
        {
            fragment_index_temp.template_whole_match_index.push_back(index_id);
            break;
        }
        case HEAD_AND_SLOT_TEMPLATE:
        {
            fragment_index_temp.template_head_and_slot_match_index.push_back(index_id);
            break;
        }
        default:
        {
            break;
        }
    }

    return RET_SUCCESS;
}

/*! @function
********************************************************************************
<PRE>
函数名   :Load_Template_File(IN const string data_file)
功能     :将模板文件载入到内存中，存储数据结构为多级双数组
参数     :IN const string data_file  模板的文件名
返回值   :成功   1；失败    0
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :




典型用法 :
--------------------------------------------------------------------------------
作者     : hankscai 2013.3.16
</PRE>
*******************************************************************************/
int C_Template_Match::Load_Template_File(IN const string& data_file)
{
    ifstream inputfile(data_file.c_str());
    //LOG(ERROR)<<data_file;
    if(!inputfile)
    {
        cout<<"Open file fail"<<endl;
        return RET_ERROR;
    }
    else
    {
        string each_line;
// =========================================================================
// = vec_fragment_corresponding_template_and_type 是一个中间变量
        vector<Fragment_Corresponding_Template_Index_Tranform_Structure>  vec_fragment_corresponding_template_and_type[MAX_FRAGMENT_INDEX_LEVEL];

        uint32_t count_line_of_template_file = 0;

        while(getline(inputfile,each_line))
        {
            each_line = StringTrimRight(each_line);

            vector<string> vec;

            SplitString(each_line, "\t", &vec);
            bool is_template_format_correct = (vec.size()==2);
            if(is_template_format_correct)
            {
                // split each line  template    weight  type
                Template_Property_Info template_property_info_temp;
                string str_template;
                str_template = vec[1];

                uint64_t template_id;
                StringToNumber(vec[0].c_str(),&template_id);

                template_property_info_temp.index_id = template_id;
                template_property_info_temp.str_template = str_template;


                if(C_Template_Match::Turn_raw_template_into_processed_format(IN str_template,
                OUT template_property_info_temp.key_template,OUT template_property_info_temp.vec_restrict_fragment_type, Is_Vec_Item_Slot) ==RET_ERROR)
                {
                    return RET_ERROR;
                }

                C_Template_Match::Estimate_The_Min_Max_Match_Term_Num_Of_Template(IN OUT template_property_info_temp);

                size_t vec_fragment_size = template_property_info_temp.key_template.size();
                //限定词的个数小于MAX_FRAGMENT_INDEX_LEVEL (10)
                if(vec_fragment_size < MAX_FRAGMENT_INDEX_LEVEL)
                {


                    int count_fragment_level = 0;
                    for(size_t i =0; i < vec_fragment_size;i++)
                    {

                           Fragment_Corresponding_Template_Index_Tranform_Structure vec_transform_fragment_index_and_type_temp;


                           C_Template_Match::Turn_Vec_Template_Into_String(template_property_info_temp.key_template[i], vec_transform_fragment_index_and_type_temp.str_fragment, vec_transform_fragment_index_and_type_temp.term_num_in_corresponding_fragment);

                           vec_transform_fragment_index_and_type_temp.index = count_line_of_template_file;

                           vec_transform_fragment_index_and_type_temp.fragment_index_type = template_property_info_temp.vec_restrict_fragment_type[i];

                            if(count_fragment_level < MAX_FRAGMENT_INDEX_LEVEL)
                            {
                                    vec_fragment_corresponding_template_and_type[count_fragment_level].push_back(vec_transform_fragment_index_and_type_temp);
                            }
                            count_fragment_level = count_fragment_level + 1;

                      }

                      count_line_of_template_file = count_line_of_template_file + 1;
                      m_Template_list.push_back(template_property_info_temp);
                }

              }
        }

        // sort the fragment
        for(int i = 0; i < MAX_FRAGMENT_INDEX_LEVEL; i++)
        {
            sort(vec_fragment_corresponding_template_and_type[i].begin(),vec_fragment_corresponding_template_and_type[i].end(),Comp_Fragment);
        }


        for(int i = 0;i < MAX_FRAGMENT_INDEX_LEVEL; i++)
        {
                    vector<Fragment_Corresponding_Template_Index_Tranform_Structure> vec_transform_fragment_index_and_type_temp;
                    vec_transform_fragment_index_and_type_temp = vec_fragment_corresponding_template_and_type[i];
                    vector<const char *>  words_vec;
                    Fragment_Index fragment_index_temp;


                    string prev_fragment = "";

                    int elem_index_size = vec_transform_fragment_index_and_type_temp.size();
                    for(int j = 0; j < elem_index_size; j++)
                    {

                        if(vec_transform_fragment_index_and_type_temp[j].str_fragment != prev_fragment)
                        {
                             if(j > 0)
                            {
        // =========================================================================
        // 对非结束索引和结束索引进行排序

                                C_Template_Match::Push_Corresponding_Fragment_Index_Into_Vec_Based_On_Fragment_Position(IN fragment_index_temp, IN i);
                            }
                            prev_fragment = vec_transform_fragment_index_and_type_temp[j].str_fragment;
                            NOTE
                            char *tmp = new char[prev_fragment.size()+1];
                            strcpy(tmp, prev_fragment.c_str());
                            words_vec.push_back(tmp);

                            C_Template_Match::Initialize_Fragment_Index_Temp(IN vec_transform_fragment_index_and_type_temp[j].str_fragment,
                                                                            IN vec_transform_fragment_index_and_type_temp[j].term_num_in_corresponding_fragment,
                                                                            IN OUT  fragment_index_temp);
                        }

                        C_Template_Match::Set_Corresponding_Index_Vec_Based_On_Fragment_Index_Types(IN vec_transform_fragment_index_and_type_temp[j].fragment_index_type
                                                                                                    ,IN vec_transform_fragment_index_and_type_temp[j].index
                                                                                                    ,OUT fragment_index_temp);

        // =========================================================================
        // 处理两种极端情况，1.整个序列的长度为1   2.处理到了最后一个元素   A 1 ; A 2 A 3 B 1 B 4

                        if((elem_index_size == 1)||((j == elem_index_size-1)&&(elem_index_size > 1)))
                        {
                            C_Template_Match::Push_Corresponding_Fragment_Index_Into_Vec_Based_On_Fragment_Position(IN fragment_index_temp, IN i);
                            break;
                        }
                    }


                    if(words_vec.size() >0)
                    {
                        m_gTrie[i].Build(words_vec.size(), &words_vec[0]);
                    }
                    else
                    {
                        //m_iLevel代表实际的级数
                        m_iLevel = i;
                        break;
                    }
                    size_t words_vec_size = words_vec.size();
                    for(size_t k = 0; k <words_vec_size;k++)
                    {

                        delete [] words_vec[k];
                    }
        }
    }
    return RET_SUCCESS;
}


int C_Template_Match::Turn_Vec_Template_Into_String(IN const vector<string>& vec_template,OUT string& str_key, OUT int& term_num_in_corresponding_fragment)
{
    size_t vec_template_size = vec_template.size();
    string output="";
    for(size_t i =0; i < vec_template_size;i++)
    {
        output= output+vec_template[i];
    }
    str_key = output;
    term_num_in_corresponding_fragment = Get_Segment_Term_Num(output);
     return RET_SUCCESS;

 }
/*! @function
********************************************************************************
<PRE>
函数名   :Turn_raw_template_into_processed_format(IN const vector<string> vec_template, OUT int& min_term_num, OUT int& max_term_num)
功能     :估计模板匹配的查询的最少和最多词的个数
参数     :模板
返回值   :模板匹配的查询的最少和最多词的个数
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :O(n)
备注     :(([^\\s]*?/[a-z]{1,2})\\s){0,2}  通过正则表达式对这类变长型slot的后缀匹配，返回变长的范围
典型用法 :
--------------------------------------------------------------------------------
作者     : hankscai
</PRE>
*******************************************************************************/


int C_Template_Match::Turn_raw_template_into_processed_format(IN const string& str_template, OUT vector<vector<string> >& vec_template,
                                                              OUT vector<Template_Index_Types_Based_On_Restrict_Fragment>& vec_fragment_index_type,IN bool (*fun_Is_Vec_Item_Slot)(IN const string& vec_item,OUT string& restrict_terms))
{

        vector<string>  vec_str_template;
        vec_str_template.clear();
        vec_fragment_index_type.clear();
        vec_template.clear();
        SplitString(str_template, " ", &vec_str_template);
        int  vec_str_template_size = vec_str_template.size();

        vector<string> vec_temp;
        vector<vector<int> > vec_int_type;

        vector<int>  vec_int_temp;

        int count_slot_term = 0; //slot的个数，slot个数为0，表示这个模板没有slot


        for(int i =0; i < vec_str_template_size;i++)
        {
         //   cout << i <<endl;
            //第i个位置的元素是一个slot（空槽）
           // if(vec_str_template[i] == "*")
           string restrict_terms;
           if((*fun_Is_Vec_Item_Slot)(vec_str_template[i],restrict_terms))
            {
                if(vec_temp.size()>0)
                {
                        vec_template.push_back(vec_temp);
                        vec_int_type.push_back(vec_int_temp);
                        vec_temp.clear();
                        vec_int_temp.clear();
                }
                count_slot_term = count_slot_term +1;

            }
            else
            {

                vec_temp.push_back(restrict_terms);
                vec_int_temp.push_back(i);

                if(i == (vec_str_template_size-1))
                {
                        vec_template.push_back(vec_temp);
                        vec_int_type.push_back(vec_int_temp);
                        vec_temp.clear();
                        vec_int_temp.clear();
                       // count_slot_term = count_slot_term + 1;
                }

            }

        }
        //LOG(ERROR) << vec_int_type.size();
        if(vec_int_type.size() == vec_template.size())
        {

            size_t vec_int_type_size = vec_int_type.size();
            for(size_t i =0; i < vec_int_type_size; i++)
            {
                vec_int_temp = vec_int_type[i];

                int min_position_num_of_fragment=vec_str_template_size;
                int max_position_num_of_fragment=-1;

                size_t vec_int_temp_size = vec_int_temp.size();

                for(size_t j = 0;j < vec_int_temp_size; j++)
                {
                        if(vec_int_temp[j] < min_position_num_of_fragment)
                        {
                            min_position_num_of_fragment = vec_int_temp[j];
                        }
                        if(vec_int_temp[j] > max_position_num_of_fragment)
                        {
                            max_position_num_of_fragment = vec_int_temp[j];
                        }

                }
                bool is_slot_num_zero = (count_slot_term == 0);
                if(is_slot_num_zero)
                {
                    vec_fragment_index_type.push_back(WHOLE_TEMPLATE);
                    continue;
                }

                bool is_i_th_fragment_beginning_of_template = (min_position_num_of_fragment == 0 && i ==0 && i < (vec_int_type_size-1));
                if(is_i_th_fragment_beginning_of_template)
                {
                    vec_fragment_index_type.push_back(BEGINNING_TEMPLATE);
                    continue;
                }
                //A * this is a head and slot template
                bool is_a_head_and_slot_template = (min_position_num_of_fragment == 0 && i ==0 && i == (vec_int_type_size-1));
                if(is_a_head_and_slot_template)
                {
                    vec_fragment_index_type.push_back(HEAD_AND_SLOT_TEMPLATE);
                    continue;
                }

                bool is_i_th_fragment_ending_of_template = ((max_position_num_of_fragment == (vec_str_template_size -1)) && (i == vec_int_type_size-1));

                if(is_i_th_fragment_ending_of_template)
                {
                     vec_fragment_index_type.push_back(ENDING_TEMPLATE);
                     continue;
                }


                bool is_i_th_fragment_the_end_of_ending_with_slot_template =
                ((max_position_num_of_fragment < (vec_str_template_size -1)) && (i == vec_int_type_size-1));

                if(is_i_th_fragment_the_end_of_ending_with_slot_template)
                {
                     vec_fragment_index_type.push_back(ENDING_WITH_SLOT_TEMPLATE);
                     continue;
                }

                bool is_i_th_forward_fragment_of_template = ( i < vec_int_type_size-1);
                if(is_i_th_forward_fragment_of_template)
                {
                     vec_fragment_index_type.push_back(FORWARDING_TEMPLATE);
                     continue;
                }



            }
        }
        else
        {
            return RET_ERROR;
        }

    return RET_SUCCESS;

}


/*! @function
********************************************************************************
<PRE>
函数名   :Estimate_The_Min_Max_Match_Term_Num_Of_Template(IN const vector<string> vec_template, OUT int& min_term_num, OUT int& max_term_num)
功能     :估计模板匹配的查询的最少和最多词的个数
参数     :模板
返回值   :模板匹配的查询的最少和最多词的个数
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :O(n)
备注     :(([^\\s]*?/[a-z]{1,2})\\s){0,2}  通过正则表达式对这类变长型slot的后缀匹配，返回变长的范围
典型用法 :
--------------------------------------------------------------------------------
作者     : hankscai
</PRE>
*******************************************************************************/

int C_Template_Match::Estimate_The_Min_Max_Match_Term_Num_Of_Template(IN OUT Template_Property_Info& template_property_info_temp)
{
        vector<string>  vec_template;
        SplitString(template_property_info_temp.str_template, " ", &vec_template);
        Regex re_matched_term_num("(.*?)\\)\\{(\\d+)\\,(\\d+)\\}$");
	Regex re_slot_pos("(.*?)/(.*?)\\)$");
        int min_term_num_temp = 0;
        int max_term_num_temp = 0;
        size_t vec_template_size = vec_template.size();
        for(size_t i =0; i < vec_template_size;i++)
        {
            string str_temp;
            int    min_token = 0;
            int    max_token = 1;
            string str_match = vec_template[i];

            if(re_matched_term_num.FullMatch(str_match, &str_temp,&min_token,&max_token))
            {
            //LOG(ERROR) << str_match;
                min_term_num_temp = min_term_num_temp + min_token;
                max_term_num_temp = max_term_num_temp + max_token;
            }
            else
            {
                min_term_num_temp = min_term_num_temp + 1;
                max_term_num_temp = max_term_num_temp + 1;
            }
	    string str_pos;
	    if(re_slot_pos.FullMatch(str_match,&str_temp,&str_pos))
	    {
	    	bool b_has_restrict_pos =(str_pos.size()>0);
	    	if(b_has_restrict_pos)
	   	{
		    map<string,int>::iterator map_it_temp = template_property_info_temp.pos_restrict.find(str_pos);
		    if(map_it_temp != template_property_info_temp.pos_restrict.end())
		    {
			map_it_temp->second = map_it_temp->second + 1;
		    }
		    else
		    {
			template_property_info_temp.pos_restrict.insert(map<string, int>::value_type(str_pos,1));
		    }
		}
	    }

    }
    template_property_info_temp.min_matched_term_num = min_term_num_temp;
    template_property_info_temp.max_matched_term_num = max_term_num_temp;


    return RET_SUCCESS;
}



/*! @function
********************************************************************************
<PRE>
函数名   :
功能     :
参数     :
返回值   :
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/


int C_Template_Match::Show_Me(IN const string& str_input)
{

    return 1;
}

vector<uint32_t> set_intersection_vec_template(IN const vector<uint32_t>& input_1,IN const vector<uint32_t>& input_2)
{
    vector<uint32_t> g_temp;
    g_temp.clear();
    set_intersection(input_1.begin(), input_1.end(),
    input_2.begin(), input_2.end(),
    back_inserter(g_temp));
    return g_temp;
}


vector<uint32_t> set_union_vec_template(const vector<uint32_t>& input_1,const vector<uint32_t>& input_2)
{
    vector<uint32_t> g_temp;
    g_temp.clear();
    set_union(input_1.begin(), input_1.end(),
    input_2.begin(), input_2.end(),
     back_inserter(g_temp));
    return g_temp;
}

int Copy_Return_Template_Info(IN const Return_Template_Info& return_info_input,IN OUT Return_Template_Info& return_info_output)
{
    return_info_output.term_num_in_corresponding_fragment = return_info_input.term_num_in_corresponding_fragment;
  //  return_info.template_index_id = 0;
    int len =0;
    GET_ARRAY_LEN(return_info_input.position,len)
    for(int i =0; i < len; i++)
    {
        return_info_output.position[i] =  return_info_input.position[i];
    }
    return RET_SUCCESS;
}

int Init_Return_Template_Info(IN OUT Return_Template_Info& return_info)
{
    return_info.term_num_in_corresponding_fragment = 0;
    int len =0;
    GET_ARRAY_LEN(return_info.position,len)
     for(int i =0; i < len; i++)
    {
        return_info.position[i] = -1;
    }
    return RET_SUCCESS;
}
/*! @function
********************************************************************************
<PRE>
函数名   :Get_Rough_Template_From_Index_First_Step_Match
功能     :通过分析输入的串，返回可能对应的模板的编号
参数     :IN const string str_query_with_no_pos  输入串
返回值   :OUT vector<uint32_t>& g_matched_templates 可能匹配模板编号
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/


//用户输入查询分词后：
int  C_Template_Match::Set_Fragment_Type_Based_On_Position_In_Query(IN const int& vec_size, IN OUT  Fragment_Info& fragment_info_temp)
{
    if(fragment_info_temp.begin_position == 0)
    {
         fragment_info_temp.match_type = HEAD_MATCH;
	 return RET_SUCCESS;
    }
    if(fragment_info_temp.end_position == (vec_size-1))
    {
         fragment_info_temp.match_type = END_MATCH;
	 return RET_SUCCESS;
    }
    if((fragment_info_temp.begin_position >0)&&(fragment_info_temp.end_position < (vec_size-1)))
    {
        fragment_info_temp.match_type = MIDDLE_MATCH;
	return RET_SUCCESS;
    }
    if((fragment_info_temp.end_position == (vec_size-1))&&(fragment_info_temp.begin_position == 0))
    {
        fragment_info_temp.match_type = WHOLE_MATCH;
        return RET_SUCCESS;
    }
    return RET_SUCCESS;
}
int C_Template_Match::Get_Matched_Fragment_Of_Each_Level_From_Raw_Query(IN const string str_query_with_no_pos,
                                                                        OUT vector<Fragment_Info> (&vec_matched_fragment_on_i_th_level)[MAX_FRAGMENT_INDEX_LEVEL])
{
     vector<string> vec;
    SplitString(str_query_with_no_pos, " ", &vec);
    int vec_size = vec.size();

    int begin_match_position = 0;

    for(int i_index_level = 0; i_index_level < MAX_FRAGMENT_INDEX_LEVEL; i_index_level++)
    {
         string str_search_string="";
        int begin_match_position_temp = vec_size;
        for(int i = (vec_size-1); i >= begin_match_position; i--)
        {
            str_search_string = vec[i] + str_search_string;

            size_t max_match_size = m_gIndex[i_index_level].size();

            int* result = new int[max_match_size + 1];

            size_t num = m_gTrie[i_index_level].CommonPrefixSearch(str_search_string.c_str(), result, max_match_size);

            for(size_t j = 0; j < num ;j++)
            {
                Fragment_Info fragment_info_temp;
                fragment_info_temp.str_fragment = m_gIndex[i_index_level][result[j]].str_fragment;
                fragment_info_temp.i_index_level = i_index_level;
                fragment_info_temp.term_num_in_corresponding_fragment = m_gIndex[i_index_level][result[j]].term_num_in_corresponding_fragment;
                fragment_info_temp.begin_position = i;
                fragment_info_temp.end_position = i + m_gIndex[i_index_level][result[j]].term_num_in_corresponding_fragment -1;
                fragment_info_temp.index_template = result[j];

		C_Template_Match::Set_Fragment_Type_Based_On_Position_In_Query(IN vec_size,IN OUT fragment_info_temp);
                vec_matched_fragment_on_i_th_level[i_index_level].push_back(fragment_info_temp);

                if(fragment_info_temp.end_position < begin_match_position_temp)
                {
                    begin_match_position_temp = fragment_info_temp.end_position;
                }


            }
             delete [] result;
        }
        begin_match_position = begin_match_position_temp;
    }
    return RET_SUCCESS;
}

int C_Template_Match::Search_Candidate_Template_Basd_On_Matched_Fragment_Stack(IN const vector<Fragment_Info> (&vec_matched_fragment_on_i_th_level)[MAX_FRAGMENT_INDEX_LEVEL],OUT vector<uint32_t>& g_output_final)
{
    struct timeval tvafter,tvpre;
    struct timezone tz;
    gettimeofday (&tvpre , &tz);

     int i_index_level = 0;
    deque<Matched_Fragment_Candidate>  matched_fragment_searching_for_candidate_template_stack;

    for(size_t i = 0; i < vec_matched_fragment_on_i_th_level[i_index_level].size(); i++)
    {
        Matched_Fragment_Candidate  matched_fragment_candidate_temp;

        Init_Return_Template_Info(matched_fragment_candidate_temp.return_info);

        matched_fragment_candidate_temp.key_infomation = vec_matched_fragment_on_i_th_level[i_index_level][i];
        if(matched_fragment_candidate_temp.key_infomation.match_type == HEAD_MATCH)
        {
            matched_fragment_candidate_temp.template_forward_index =  m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_begin_index;
            NOTE //
            matched_fragment_candidate_temp.template_ending_index =  m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_head_and_slot_match_index;

        }
        if(matched_fragment_candidate_temp.key_infomation.match_type == MIDDLE_MATCH)
        {
            matched_fragment_candidate_temp.template_forward_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_forward_index;
            matched_fragment_candidate_temp.template_ending_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_end_with_slot_index;

          //  matched_fragment_candidate_temp.template_output_index = vec_matched_fragment_on_i_th_level[i_index_level][i].template_forward_index;
        }
        if(matched_fragment_candidate_temp.key_infomation.match_type == END_MATCH)
        {
            matched_fragment_candidate_temp.template_ending_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_end_index;
            //模板没有slot
            // matched_fragment_candidate_temp.template_forward_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].;

        }

        if(matched_fragment_candidate_temp.key_infomation.match_type == WHOLE_MATCH)
        {
            matched_fragment_candidate_temp.template_ending_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].template_whole_match_index;
            //模板没有slot
            // matched_fragment_candidate_temp.template_forward_index = m_gIndex[i_index_level][vec_matched_fragment_on_i_th_level[i_index_level][i].index_template].;

        }


        matched_fragment_candidate_temp.return_info.position[matched_fragment_candidate_temp.return_info.term_num_in_corresponding_fragment] = matched_fragment_candidate_temp.key_infomation.begin_position;

        if((matched_fragment_candidate_temp.return_info.term_num_in_corresponding_fragment +1) <MAX_FRAGMENT_INDEX_LEVEL)
        {
            matched_fragment_candidate_temp.return_info.term_num_in_corresponding_fragment = matched_fragment_candidate_temp.return_info.term_num_in_corresponding_fragment +1;
        }


        matched_fragment_searching_for_candidate_template_stack.push_back(matched_fragment_candidate_temp);
    }

     gettimeofday (&tvafter , &tz);
     LOG(ERROR) <<"2 /  3 in first step"<<((tvafter.tv_sec-tvpre.tv_sec)*1000000+(tvafter.tv_usec-tvpre.tv_usec));
     gettimeofday (&tvpre , &tz);

    vector<uint32_t> g_output;


    while(!matched_fragment_searching_for_candidate_template_stack.empty())
    {
        Matched_Fragment_Candidate  matched_fragment_candidate_temp;
        matched_fragment_candidate_temp  = matched_fragment_searching_for_candidate_template_stack.back();
        matched_fragment_searching_for_candidate_template_stack.pop_back();
        i_index_level = matched_fragment_candidate_temp.key_infomation.i_index_level + 1;

        //LOG(ERROR)<<matched_fragment_candidate_temp.key_infomation.str_fragment;
        //LOG(ERROR)<<matched_fragment_candidate_temp.key_infomation.match_type;

        if(i_index_level<(m_iLevel+1) &&(i_index_level<MAX_FRAGMENT_INDEX_LEVEL))
        {
             vector<Fragment_Info>   vec_fragment_info_temp;
             vec_fragment_info_temp = vec_matched_fragment_on_i_th_level[i_index_level];
             size_t vec_key_info_temp_size = vec_fragment_info_temp.size();

            for(size_t i= 0;i<vec_key_info_temp_size;i++)
            {
                Fragment_Info key_info_temp;

                key_info_temp = vec_fragment_info_temp[i];

                if(key_info_temp.begin_position > (matched_fragment_candidate_temp.key_infomation.end_position + 1))//*必须加东西
                {

                    Matched_Fragment_Candidate  Template_Deque_new_temp;

                    Copy_Return_Template_Info(matched_fragment_candidate_temp.return_info,Template_Deque_new_temp.return_info);

                    Template_Deque_new_temp.key_infomation = key_info_temp;

                    //结束条件
                    if(key_info_temp.match_type == END_MATCH)
                    {

                            g_output = set_intersection_vec_template(m_gIndex[key_info_temp.i_index_level][key_info_temp.index_template].template_forward_index,matched_fragment_candidate_temp.template_forward_index);

                            Template_Deque_new_temp.template_forward_index = g_output;
                            NOTE
                            g_output = set_intersection_vec_template(m_gIndex[key_info_temp.i_index_level][key_info_temp.index_template].template_end_index,matched_fragment_candidate_temp.template_forward_index);

                            Template_Deque_new_temp.template_ending_index = g_output;

                    }
                    else
                    {
                        g_output = set_intersection_vec_template(m_gIndex[key_info_temp.i_index_level][key_info_temp.index_template].template_forward_index,matched_fragment_candidate_temp.template_forward_index);
                        Template_Deque_new_temp.template_forward_index = g_output;

                         g_output = set_intersection_vec_template(m_gIndex[key_info_temp.i_index_level][key_info_temp.index_template].template_end_with_slot_index,matched_fragment_candidate_temp.template_forward_index);
                        Template_Deque_new_temp.template_ending_index = g_output;
                    }

                    if((Template_Deque_new_temp.template_forward_index.size() > 0)||(Template_Deque_new_temp.template_ending_index.size() > 0))
                    {

                           Template_Deque_new_temp.return_info.position[Template_Deque_new_temp.return_info.term_num_in_corresponding_fragment] = key_info_temp.begin_position;
                           if((Template_Deque_new_temp.return_info.term_num_in_corresponding_fragment +1) <MAX_FRAGMENT_INDEX_LEVEL)
                           {
                               Template_Deque_new_temp.return_info.term_num_in_corresponding_fragment = Template_Deque_new_temp.return_info.term_num_in_corresponding_fragment +1;
                           }

                           matched_fragment_searching_for_candidate_template_stack.push_back(Template_Deque_new_temp);

                    }
                }
            }
            //结束条件
            g_output_final = set_union_vec_template(matched_fragment_candidate_temp.template_ending_index,g_output_final);
            matched_fragment_candidate_temp.return_info.template_index_id = matched_fragment_candidate_temp.template_ending_index;
        }
      }

     gettimeofday (&tvafter , &tz);
     LOG(ERROR) <<"3 /  3 in first step"<<((tvafter.tv_sec-tvpre.tv_sec)*1000000+(tvafter.tv_usec-tvpre.tv_usec));
    return RET_SUCCESS;
}


int C_Template_Match::Get_Rough_Template_From_Index_First_Step_Match(IN const string str_query_with_no_pos, OUT vector<uint32_t>& g_matched_templates)
{


    vector<Fragment_Info> vec_matched_fragment_on_i_th_level[MAX_FRAGMENT_INDEX_LEVEL];
      struct timeval tvafter,tvpre;
    struct timezone tz;
    gettimeofday (&tvpre , &tz);

   C_Template_Match::Get_Matched_Fragment_Of_Each_Level_From_Raw_Query(IN str_query_with_no_pos,
                                                                        OUT vec_matched_fragment_on_i_th_level);
    gettimeofday (&tvafter , &tz);

     LOG(ERROR) <<"1 /  3 in first step"<<((tvafter.tv_sec-tvpre.tv_sec)*1000000+(tvafter.tv_usec-tvpre.tv_usec));


    vector<uint32_t> g_output_final;

    C_Template_Match::Search_Candidate_Template_Basd_On_Matched_Fragment_Stack(IN vec_matched_fragment_on_i_th_level,OUT g_output_final);


    g_matched_templates = g_output_final;

    return RET_SUCCESS;

}
/*! @function
********************************************************************************
<PRE>
函数名   :Get_Exact_Match_Template(IN const string str_query_with_pos,IN vector<uint32_t> g_matched_templates, OUT vector<Template_Property_Info>& g_exact_match_template)
功能     :
参数     :
返回值   :
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/


int C_Template_Match::Get_Exact_Match_Template_From_Feedback_Second_Step_Match(IN const string& str_query_with_pos,IN vector<uint32_t>& g_matched_templates,
                                                                                     OUT vector<Template_Property_Info>& g_exact_match_template)
{
    g_exact_match_template.clear();
    size_t Vec_Size = g_matched_templates.size();
    size_t Template_Size = m_Template_list.size();
    string str_template;
    uint32_t  index_template;

    Regex::Options options;
    options.SetAllOptions(PCRE_CASELESS | PCRE_DOTALL);
    options.SetDotAll(true);
    options.SetUngreedy(true);


 //   FOR_DBG
//     LOG(ERROR) << Vec_Size;
 //   LOG(ERROR) << Template_Size;
    int  count_sum =0;

    for(size_t i = 0; i < Vec_Size; i++)
    {
        index_template = g_matched_templates[i];
        if( index_template < Template_Size)
        {


    	    vector<string> vec;

            SplitString(str_query_with_pos, " ", &vec);

            int vec_size = vec.size();
/*
            LOG(ERROR) <<"vec_size "<<vec_size;
            LOG(ERROR) << str_template;

            LOG(ERROR) <<"min "<<m_Template_list[index_template].min_matched_term_num;
            LOG(ERROR) <<"max "<<m_Template_list[index_template].max_matched_term_num;

*/
            bool is_query_term_num_matched_template_term_num = ((vec_size >= m_Template_list[index_template].min_matched_term_num)
                            &&(vec_size <= m_Template_list[index_template].max_matched_term_num));

            if (is_query_term_num_matched_template_term_num)
            {

            	str_template = m_Template_list[index_template].str_template;
            	Regex re(str_template,options);

            //     LOG(ERROR) << str_query_with_pos;
             //     LOG(ERROR) << str_template;


                count_sum = count_sum+1;
                bool is_query_matched_template =re.FullMatch(str_query_with_pos);
                 if(is_query_matched_template)
                {
                    LOG(ERROR) <<"matched";
                    Template_Property_Info  template_property_info_temp;
                    template_property_info_temp = m_Template_list[index_template];
                    g_exact_match_template.push_back(template_property_info_temp);
                }
                else
                {
                    LOG(ERROR) <<"not matched";
                    LOG(ERROR) <<index_template<<" " <<str_template;
                    //LOG(ERROR) << str_query_with_pos;

                }
            }
            else
            {
                //LOG(ERROR) <<index_template<<" "<< str_template;
/*
                 LOG(WARNING) << str_template;
                 LOG(WARNING) << str_query_with_pos;
                 LOG(WARNING) << m_Template_list[index_template].min_matched_term_num;
                 LOG(WARNING) << m_Template_list[index_template].max_matched_term_num;
                 LOG(WARNING) << vec_size;
*/
            }

        }

    }
    LOG(ERROR) << count_sum;

    return RET_SUCCESS;
}


/*! @function
********************************************************************************
<PRE>
函数名   :Get_Seg_Result(IN const string str_query, IN bool bpos,OUT string& str_seg)
功能     :template_property_info_temp
参数     :
返回值   :
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/


int C_Template_Match::Get_Seg_Result(IN const string str_query, OUT string& query_seg_no_pos, OUT string& query_seg_pos)
{
    query_seg_pos.clear();
    query_seg_no_pos.clear();
    if (TCSegment(seg_handle, str_query.c_str()))
    {
        int   rescount;                                 // 词条个数
        rescount = TCGetResultCnt(seg_handle);

           // 分词的同时获取词性信息
        for (int i=0; i<rescount; i++)
        {
                char *word;                                     // 切词结果
                pWP   wordpos;                                  // 词和词性
                wordpos = TCGetAt(seg_handle, i);
                char  pos[16];
                TCPosId2Str(wordpos->pos, pos);
                char out[100];
                snprintf(out, sizeof(out),"%s/%s", wordpos->word, pos);
                query_seg_pos = query_seg_pos + out + " ";

                word = TCGetWordAt(seg_handle, i);
                query_seg_no_pos = query_seg_no_pos +  word + " ";

        }

        query_seg_pos = StringTrim(query_seg_pos);
        query_seg_no_pos = StringTrim(query_seg_no_pos);
    }
    return RET_SUCCESS;

}


int C_Template_Match::Get_Segment_Term_Num(IN const string& str_query)
{
    int rescount = 0;
    if (TCSegment(seg_handle, str_query.c_str()))
    {
        rescount = TCGetResultCnt(seg_handle);
    }
    return rescount;

}


/*! @function
********************************************************************************
<PRE>
函数名   :Get_Match_Template(IN const string str_query, OUT vector<Template_Property_Info>& g_exact_match_template)
功能     :
参数     :
返回值   :
抛出异常 :
--------------------------------------------------------------------------------
复杂度   :
备注     :
典型用法 :
--------------------------------------------------------------------------------
作者     : <xxx>
</PRE>
*******************************************************************************/


int C_Template_Match::Get_Match_Template(IN const string& str_query, OUT vector<Template_Property_Info>& g_exact_match_template)
{
    string query_seg_no_pos;
    string query_seg_pos;


    C_Template_Match::Get_Seg_Result(str_query,query_seg_no_pos,query_seg_pos);





    g_exact_match_template.clear();

   // vector<u_> g_matched_templates
   vector<uint32_t> g_matched_templates;

   struct timeval tvafter,tvpre;
    struct timezone tz;
    gettimeofday (&tvpre , &tz);

    C_Template_Match::Get_Rough_Template_From_Index_First_Step_Match(query_seg_no_pos, g_matched_templates);

     gettimeofday (&tvafter , &tz);

     LOG(ERROR) <<"first step"<<((tvafter.tv_sec-tvpre.tv_sec)*1000000+(tvafter.tv_usec-tvpre.tv_usec));

     gettimeofday (&tvpre , &tz);
//    LOG(ERROR) << g_matched_templates.size();

    C_Template_Match::Get_Exact_Match_Template_From_Feedback_Second_Step_Match(IN query_seg_pos,IN g_matched_templates,
                                                                                     OUT g_exact_match_template);

    gettimeofday (&tvafter , &tz);

     LOG(ERROR) <<"second step"<<((tvafter.tv_sec-tvpre.tv_sec)*1000000+(tvafter.tv_usec-tvpre.tv_usec));


    return RET_SUCCESS;
}

int C_Template_Match::Get_Match_Template_Info(IN const string& str_query, OUT vector<pair<uint64_t,string> >& g_match_template_info)
{
        if(str_query.length()<=0)
        {
            return RET_ERROR;
        }

        if(NULL == seg_handle)
        {
            return RET_ERROR;
        }

        string str_seg;

        string str_seg_no_pos;

        C_Template_Match::Get_Seg_Result(IN str_query, OUT str_seg_no_pos, OUT str_seg);



        // vector<u_> g_matched_templates
        vector<uint32_t> g_matched_templates;
        g_matched_templates.clear();

        C_Template_Match::Get_Rough_Template_From_Index_First_Step_Match(str_seg_no_pos, g_matched_templates);
//        LOG(ERROR) << g_matched_templates.size();

        vector<Template_Property_Info> g_exact_match_template;
        g_exact_match_template.clear();

        C_Template_Match::Get_Exact_Match_Template_From_Feedback_Second_Step_Match(IN str_seg,IN g_matched_templates, OUT g_exact_match_template);

        size_t g_exact_match_template_size = g_exact_match_template.size();
        for(size_t i  =0; i < g_exact_match_template_size; i++)
        {
            Template_Property_Info Template_Property_Info_temp;
            Template_Property_Info_temp = g_exact_match_template[i];
            g_match_template_info.push_back(make_pair<uint64_t,string>(Template_Property_Info_temp.index_id,Template_Property_Info_temp.str_template));


        }
        return RET_SUCCESS;
}
