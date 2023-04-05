#include <aoi_manager.h>
#include <list_aoi.h>
#include <grid_aoi.h>
#include <brute_aoi.h>
#include "set_utility.h"
#include <random>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include "pretty_print.h"
#include "set_utility.h"
#include <nlohmann/json.hpp>
#include <sstream>
using json = nlohmann::json;
using namespace spiritsaway::aoi;
std::vector<std::unordered_map<guid_t, std::array<std::unordered_set<guid_t>, 2>>> aoi_results(10);


template<typename T>
void clear_subvec(std::vector<T>& data)
{
	for(auto& one_vec: data)
	{
		one_vec.clear();
	}
}
pos_t create_pos(pos_unit_t a, pos_unit_t b, pos_unit_t c)
{
	pos_t result;
	result[0] = a;
	result[1] = b;
	result[2] = c;
	return result;
}
void dump_input(const std::vector<pos_t>& guid_positions, const std::vector<pos_unit_t>& guid_radius)
{
	std::ofstream file_os("input.json");
	auto json_1 = json(guid_positions);
	auto json_2 = json(guid_radius);
	json json_3;
	json_3.push_back(json_1);
	json_3.push_back(json_2);
	file_os << json_3.dump(4) << std::endl;
	
}

void load_input(std::vector<pos_t>& guid_positions, std::vector<pos_unit_t>& guid_radius)
{
	std::ifstream file_is("input.json");
	std::stringstream buffer;
	buffer << file_is.rdbuf();
	auto cur_file_content = buffer.str();
	// std::cout<<"cur file_content is "<<cur_file_content<<std::endl;
	if (!json::accept(cur_file_content))
	{
		return;
	}
	auto json_3 = json::parse(cur_file_content);
	json_3[0].get_to(guid_positions);
	json_3[1].get_to(guid_radius);
}
void report_cost(std::chrono::system_clock::time_point& start, std::chrono::system_clock::time_point& end, std::string reason, int line)
{
	end = std::chrono::system_clock::now();
	auto elapsed_seconds = end - start;
	std::cout << "cost time " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count() << "ms for line " << line << " with reason "<<reason<<  std::endl;
	start = end;
}
void report_relation(guid_t guid_1, guid_t guid_2, const std::vector<pos_t>& guid_positions, const std::vector<pos_unit_t>& guid_radius)
{
	auto pos_1 = guid_positions[guid_1];
	auto pos_2 = guid_positions[guid_2];
	auto diff_x = pos_1[0] - pos_2[0];
	auto diff_z = pos_1[2] - pos_2[2];
	auto dis = diff_x * diff_x + diff_z * diff_z;

	std::cout << "guid_1 " << guid_1 << " to guid_2 " << guid_2 << " diff_x " << diff_x<<" diff_z "<<diff_z<< " range " << guid_radius[guid_1]  << std::endl;
}
bool check_on_guid_result(guid_t guid, const std::vector<pos_t>& guid_positions, const std::vector<pos_unit_t>& guid_radius, const std::array<std::unordered_set<guid_t>, 2>& a, const std::array<std::unordered_set<guid_t>, 2>& b)
{
	std::unordered_set<guid_t> diff_1_2;
	std::unordered_set<guid_t> diff_2_1;
	bool fail = false;
	unordered_set_diff(a[0], b[0], diff_1_2);
	unordered_set_diff(b[0], a[0], diff_2_1);
	if (!diff_1_2.empty())
	{
		std::cout << "enter guid diff_1_2" << std::endl;
		fail = true;
		for (auto one_item : diff_1_2)
		{
			report_relation(guid, one_item, guid_positions, guid_radius);
		}
	}
	if (!diff_2_1.empty())
	{
		std::cout << "enter guid diff_2_1" << std::endl;
		fail = true;
		for (auto one_item : diff_2_1)
		{
			report_relation(guid, one_item, guid_positions, guid_radius);
		}
	}
	diff_1_2.clear();
	diff_2_1.clear();
	unordered_set_diff(a[1], b[1], diff_1_2);
	unordered_set_diff(b[1], a[1], diff_2_1);
	if (!diff_1_2.empty())
	{
		std::cout << "leave guid diff_1_2" << std::endl;
		fail = true;
		for (auto one_item : diff_1_2)
		{
			report_relation(guid, one_item, guid_positions, guid_radius);
		}
	}
	if (!diff_2_1.empty())
	{
		std::cout << "leave guid diff_2_1" << std::endl;
		fail = true;
		for (auto one_item : diff_2_1)
		{
			report_relation(guid, one_item, guid_positions, guid_radius);
		}
	}
	return fail;
}
auto result_lambda = [](std::size_t i)
{
	return [i](guid_t guid_from, const std::vector<aoi_notify_info>& notify_info)
	{
		auto& temp = aoi_results[i][guid_from];
		for (const auto& one_info : notify_info)
		{
			if (one_info.is_enter)
			{
				temp[0].insert(one_info.other_guid);
			}
			else
			{
				temp[1].insert(one_info.other_guid);
			}
		}
	};
};

bool check_aoi_result(const std::vector<std::unordered_map<guid_t, std::array<std::unordered_set<guid_t>, 2>>>& result, const std::vector<pos_t>& guid_positions, const std::vector<pos_unit_t>& guid_radius, std::uint32_t mgr_sz)
{
	if (result.size() < 2)
	{
		return false;
	}
	//for (int i = 1; i < result.size(); i++)
	//{
	//	if (result[i].size() != result[0].size())
	//	{
	//		std::cout << "size check fail expected size " << result[0].size() << " real size " << result[i].size() <<" for index "<<i<< std::endl;
	//		return false;
	//	}
	//}
	std::unordered_set<guid_t> diff_result;
	for (auto one_item : result[0])
	{
		auto guid = one_item.first;
		auto cur_value = one_item.second;
		if (cur_value[0].empty() && cur_value[1].empty())
		{
			continue;
		}
		for (int i = 1; i < mgr_sz; i++)
		{
			auto cur_iter = result[i].find(guid);
			if (cur_iter == result[i].end())
			{
				std::cout << "cant find guid " << guid << " in result " << i << std::endl;
				return false;
			}
			if (check_on_guid_result(guid, guid_positions, guid_radius, cur_value, cur_iter->second))
			{
				std::cout << "check_on_guid_result fail guid " << guid << " in result " << i << std::endl;
				std::cout << "cur_value is " << cur_value << " other value is " << cur_iter->second << std::endl;
				return false;
			}
		}
	}
	return true;
}

std::vector<aoi_manager*> create_aoi_mgrs(std::uint32_t max_entity_size, pos_unit_t max_aoi_radius, pos_unit_t min_aoi_radius, pos_t border_min, pos_t border_max)
{
	std::uint32_t grid_block_size = 4096;
	std::uint32_t grid_size = 100;
	auto grid_impl = new grid_aoi(max_entity_size, max_aoi_radius, border_min, border_max, grid_size, grid_block_size);
	auto list_impl = new list_2d_aoi(max_entity_size, 4 * max_entity_size, max_aoi_radius, border_min, border_max);
	auto brute_impl = new brute_aoi(max_entity_size, max_aoi_radius, border_min, border_max);
	auto brute_aoi_mgr = new aoi_manager(false, brute_impl, max_entity_size, 4 * max_entity_size, border_min, border_max, result_lambda(0));
	auto grid_aoi_mgr = new aoi_manager(false, grid_impl, max_entity_size, 4 * max_entity_size, border_min, border_max, result_lambda(1));
	auto list_aoi_mgr = new aoi_manager(false, list_impl, max_entity_size, 4 * max_entity_size, border_min, border_max, result_lambda(2));

	std::vector<aoi_manager*> total_aoi_mgrs{ brute_aoi_mgr, grid_aoi_mgr, list_aoi_mgr };
	return total_aoi_mgrs;
}
void destroy_aoi_mgrs(std::vector<aoi_manager*>& mgrs)
{
	for (auto one_mgr : mgrs)
	{
		delete one_mgr;
	}
	mgrs.clear();
}
void dump_aoi_mgrs(const std::vector<aoi_manager*>& mgrs)
{
	std::string file_name = "aoi_mgr_dump";
	for (std::size_t i = 0; i < mgrs.size(); i++)
	{
		std::string file_name = "aoi_mgr_" + std::to_string(i) + ".txt";
		std::ofstream file_stream(file_name);
		mgrs[i]->dump(file_stream);
		file_stream.close();
	}
}
void dump_entity_poses(const std::vector<pos_t>& data, const std::string& filename)
{
	std::ofstream file_stream(filename);
	for (int i = 0; i < data.size(); i++)
	{
		file_stream << i << ":" << data[i][0] << " " << data[i][1] << " " << data[i][2] << std::endl;
	}
	file_stream.close();
}
std::vector<pos_t> load_entity_poses(const std::string& filename)
{
	std::ifstream file_stream(filename);
	guid_t guid;
	pos_t pos;
	char emp;
	std::vector<pos_t> result;
	while (file_stream)
	{
		file_stream >> guid >> emp >> pos[0] >> emp >> pos[1] >> emp >> pos[2];
		result.push_back(pos);
	}
	return result;

}
void dump_entity_move_info(const std::vector<std::pair<guid_t, pos_t>>& data, const std::string& filename)
{
	std::ofstream file_stream(filename);
	for (auto one_item: data)
	{
		file_stream << one_item.first << ":" << one_item.second[0] << " " << one_item.second[1] << " " << one_item.second[2] << std::endl;
	}
	file_stream.close();
}

std::vector<std::pair<guid_t, pos_t>> load_entity_move_info(const std::string& filename)
{
	std::ifstream file_stream(filename);
	guid_t guid;
	pos_t pos;
	char emp;
	std::vector<std::pair<guid_t, pos_t>> result;
	while (file_stream)
	{
		file_stream >> guid >> emp >> pos[0] >> emp >> pos[1] >> emp >> pos[2];
		result.emplace_back(guid, pos);
	}
	return result;

}
void create_entity_poses(pos_t border_min, pos_t border_max, std::uint32_t max_entity_count, pos_unit_t max_radius, std::vector<pos_t>& poses, std::vector<pos_unit_t>& radiuses, float range_scale = 0.5)
{
	std::random_device r;
	std::default_random_engine e1(r());
	std::uniform_real_distribution<float> uniform_dist(-10000.0f, 10000.0f);
	auto x_scale = range_scale* (border_max[0] - border_min[0]) / 20000;
	auto z_scale = range_scale * (border_max[2] - border_min[2]) / 20000;
	pos_t middle{ border_min[0] / 2 + border_max[0] / 2, border_min[1] / 2 + border_max[1] / 2, border_min[2] / 2 + border_max[2] / 2 };
	int half_radius = int(max_radius) / 2;
	poses = std::vector<pos_t>(max_entity_count);
	radiuses = std::vector<pos_unit_t>(max_entity_count);
	for (guid_t i = 0; i < max_entity_count; i++)
	{
		poses[i] = pos_t{ middle[0] + uniform_dist(e1) * x_scale, middle[1], middle[2] + uniform_dist(e1) * z_scale };
		radiuses[i] = (i * 1234) % half_radius + 0.2f * half_radius;
	}
	//for (std::uint32_t i = 0; i < max_entity_count; i++)
	//{
	//	poses[i] = pos_t{ 1.0f * i, 0.0f, 0.0f};
	//	radiuses[i] = 100.0f;
	//}

}
std::vector<std::pair<guid_t, pos_t>> create_entity_diffs(std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues, float percentage, std::uint32_t case_num)
{
	if (percentage <= 0 || percentage >= 1.0f)
	{
		percentage = 0.2f;
	}
	std::vector<std::pair<guid_t, pos_t>> result(case_num);
	std::random_device r;


	std::default_random_engine e1(r());
	std::uniform_real_distribution<float> move_uniform_dist(-1 * percentage, percentage);
	std::uniform_int_distribution<int> guid_uniform_dist(0, 1000000);

	for (std::size_t i = 0; i < case_num; i++)
	{
		auto cur_guid = guid_uniform_dist(e1) % entity_poses.size();
		auto cur_move_scale_x = move_uniform_dist(e1);
		auto cur_move_scale_z = move_uniform_dist(e1);
		entity_poses[cur_guid][0] += cur_move_scale_x * entity_radiues[cur_guid];
		entity_poses[cur_guid][2] += cur_move_scale_z * entity_radiues[cur_guid];
		result[i] = std::make_pair(cur_guid, entity_poses[cur_guid]);
	}
	return result;
}
bool test_add(std::vector<aoi_manager*> mgrs, const std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues)
{

	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	aoi_radius_controler cur_aoi_radius_ctrl;
	cur_aoi_radius_ctrl.min_height = cur_aoi_radius_ctrl.min_height = 0;
	cur_aoi_radius_ctrl.any_flag = 1;
	cur_aoi_radius_ctrl.forbid_flag = 0;
	cur_aoi_radius_ctrl.need_flag = 1;
	cur_aoi_radius_ctrl.max_interest_in = entity_poses.size() + 1;

	for (std::size_t i = 0; i< mgrs.size(); i++)
	{
		auto one_mgr = mgrs[i];
		for (guid_t j = 0; j < entity_poses.size(); j++)
		{
			cur_aoi_radius_ctrl.radius = entity_radiues[j];
			auto cur_pos_idx = one_mgr->add_pos_entity(j,  entity_poses[j], 1);
			one_mgr->add_radius_entity(cur_pos_idx, cur_aoi_radius_ctrl);
		}
		report_cost(start, end, "add entity for aoi_mgr " + std::to_string(i), __LINE__);

		one_mgr->update();
		report_cost(start, end, "update for aoi_mgr_" + std::to_string(i), __LINE__);
	}
	return check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size());;

}

bool test_delete(std::vector<aoi_manager*> mgrs, const std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues)
{

	std::vector<std::vector<aoi_pos_idx>> aoi_entity_idxes(mgrs.size());
	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	aoi_radius_controler cur_aoi_radius_ctrl;
	cur_aoi_radius_ctrl.min_height = cur_aoi_radius_ctrl.min_height = 0;
	cur_aoi_radius_ctrl.any_flag = 1;
	cur_aoi_radius_ctrl.forbid_flag = 0;
	cur_aoi_radius_ctrl.need_flag = 1;
	cur_aoi_radius_ctrl.max_interest_in = entity_poses.size() + 1;
	for (std::size_t i = 0; i < mgrs.size(); i++)
	{
		auto one_mgr = mgrs[i];
		for (guid_t j = 0; j < entity_poses.size(); j++)
		{
			cur_aoi_radius_ctrl.radius = entity_radiues[j];
			auto cur_pos_idx = one_mgr->add_pos_entity(j, entity_poses[j], 1);
			aoi_entity_idxes[i].push_back(cur_pos_idx);
			one_mgr->add_radius_entity(cur_pos_idx, cur_aoi_radius_ctrl);

		}

		one_mgr->update();
	}
	if (!check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size()))
	{
		std::cout << "test_delete fail in check_aoi_radius 1" << std::endl;
		dump_aoi_mgrs(mgrs);

		return false;
	}
	clear_subvec(aoi_results);
	for (guid_t j = 0; j < entity_poses.size(); j++)
	{
		for (std::size_t i = 0; i < mgrs.size(); i++)
		{
			mgrs[i]->remove_pos_entity(aoi_entity_idxes[i][j]);
			mgrs[i]->update();

		}
		if (!check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size()))
		{
			std::cout << "test_delete fail to check aoi result 2 j is " <<j<< std::endl;
			dump_aoi_mgrs(mgrs);

			return false;
		}
		clear_subvec(aoi_results);
	}
	return true;
}


bool test_trace(std::vector<aoi_manager*> mgrs, const std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues)
{


	std::vector<std::vector<aoi_pos_idx>> aoi_entity_idxes(mgrs.size());
	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	aoi_radius_controler cur_aoi_radius_ctrl;
	cur_aoi_radius_ctrl.min_height = cur_aoi_radius_ctrl.min_height = 0;
	cur_aoi_radius_ctrl.any_flag = 1;
	cur_aoi_radius_ctrl.forbid_flag = 0;
	cur_aoi_radius_ctrl.need_flag = 1;
	cur_aoi_radius_ctrl.max_interest_in = entity_poses.size() + 1;
	for (guid_t j = 0; j < entity_poses.size(); j++)
	{
		for (std::size_t i = 0; i < mgrs.size(); i++)
		{
			cur_aoi_radius_ctrl.radius = entity_radiues[j];
			auto cur_pos_idx = mgrs[i]->add_pos_entity(j, entity_poses[j], 1);
			aoi_entity_idxes[i].push_back(cur_pos_idx);
			mgrs[i]->add_radius_entity(cur_pos_idx, cur_aoi_radius_ctrl);
			mgrs[i]->update();

		}
		if (!check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size()))
		{
			std::cout << "fail to check aoi result" << std::endl;
			dump_aoi_mgrs(mgrs);
			return false;
		}
		clear_subvec(aoi_results);
	}
	return true;

}

bool test_move_trace(std::vector<aoi_manager*> mgrs, std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues,  const std::vector<std::pair<guid_t, pos_t>>& move_diffs)
{
	
	std::vector<std::unordered_map<guid_t, std::array<std::unordered_set<guid_t>, 2>>> aoi_results{ mgrs.size() };
	std::vector<std::vector<aoi_pos_idx>> aoi_entity_idxes(mgrs.size());

	aoi_radius_controler cur_aoi_radius_ctrl;
	cur_aoi_radius_ctrl.min_height = cur_aoi_radius_ctrl.min_height = 0;
	cur_aoi_radius_ctrl.any_flag = 1;
	cur_aoi_radius_ctrl.forbid_flag = 0;
	cur_aoi_radius_ctrl.need_flag = 1;
	cur_aoi_radius_ctrl.max_interest_in = entity_poses.size() + 1;
	for (std::size_t i = 0; i< mgrs.size(); i++)
	{
		auto one_mgr = mgrs[i];
		for (std::size_t j = 0; j < entity_poses.size(); j++)
		{
			cur_aoi_radius_ctrl.radius = entity_radiues[j];
			auto cur_pos_idx = one_mgr->add_pos_entity(j, entity_poses[j], 1);
			aoi_entity_idxes[i].push_back(cur_pos_idx);
			one_mgr->add_radius_entity(cur_pos_idx, cur_aoi_radius_ctrl);
		}
		one_mgr->update();
	}
	if (!check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size()))
	{
		std::cout << "check_aoi_result fail in check move " << __LINE__ << std::endl;
		dump_aoi_mgrs(mgrs);
		return false;
	}
	std::cout << "test_move add pass" << std::endl;
	clear_subvec(aoi_results);

	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	auto entity_poses_dup = entity_poses;
	for (guid_t j = 0; j < entity_poses.size(); j++)
	{

		for (std::size_t i = 0; i < mgrs.size(); i++)
		{
			auto one_mgr = mgrs[i];
			one_mgr->change_entity_pos(aoi_entity_idxes[i][move_diffs[j].first], move_diffs[j].second);
		}
		entity_poses_dup[move_diffs[j].first] = move_diffs[j].second;
		if (!check_aoi_result(aoi_results, entity_poses_dup, entity_radiues, mgrs.size()))
		{
			std::cout << "fail to check aoi result" << std::endl;
			dump_aoi_mgrs(mgrs);
			return false;
		}
		clear_subvec(aoi_results);
	}
	return true;
}
bool test_move_speed(std::vector<aoi_manager*> mgrs, std::vector<pos_t>& entity_poses, const std::vector<pos_unit_t>& entity_radiues, const std::vector<std::pair<guid_t, pos_t>>& move_diffs)
{

	std::vector<std::unordered_map<guid_t, std::array<std::unordered_set<guid_t>, 2>>> aoi_results{ mgrs.size() };
	std::vector<std::vector<aoi_pos_idx>> aoi_entity_idxes(mgrs.size());

	aoi_radius_controler cur_aoi_radius_ctrl;
	cur_aoi_radius_ctrl.min_height = cur_aoi_radius_ctrl.min_height = 0;
	cur_aoi_radius_ctrl.any_flag = 1;
	cur_aoi_radius_ctrl.forbid_flag = 0;
	cur_aoi_radius_ctrl.need_flag = 1;
	cur_aoi_radius_ctrl.max_interest_in = entity_poses.size() + 1;
	for (std::size_t i = 0; i < mgrs.size(); i++)
	{
		auto one_mgr = mgrs[i];
		for (std::size_t j = 0; j < entity_poses.size(); j++)
		{
			cur_aoi_radius_ctrl.radius = entity_radiues[j];
			auto cur_pos_idx = one_mgr->add_pos_entity(j, entity_poses[j], 1);
			aoi_entity_idxes[i].push_back(cur_pos_idx);
			one_mgr->add_radius_entity(cur_pos_idx, cur_aoi_radius_ctrl);
		}
		one_mgr->update();
	}
	if (!check_aoi_result(aoi_results, entity_poses, entity_radiues, mgrs.size()))
	{
		std::cout << "check_aoi_result fail in check move " << __LINE__ << std::endl;
		dump_aoi_mgrs(mgrs);
		return false;
	}
	std::cout << "test_move add pass" << std::endl;
	clear_subvec(aoi_results);

	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	auto entity_pos_dup = entity_poses;
	for (std::size_t i = 0; i < mgrs.size(); i++)
	{
		auto one_mgr = mgrs[i];
		for (const auto& one_item : move_diffs)
		{
			one_mgr->change_entity_pos(aoi_entity_idxes[i][one_item.first], one_item.second);
		}
		report_cost(start, end, "change_entity_pos for mgr" + std::to_string(i), __LINE__);
		one_mgr->update();
		report_cost(start, end, "change_entity_pos update for mgr" + std::to_string(i), __LINE__);
	}
	for (const auto& one_item : move_diffs)
	{
		entity_pos_dup[one_item.first] = one_item.second;
	}
	if (!check_aoi_result(aoi_results, entity_pos_dup, entity_radiues, mgrs.size()))
	{
		std::cout << "check_aoi_result fail in phase 2 " << __LINE__ << std::endl;
		dump_entity_poses(entity_poses, "begin_pos.txt");
		dump_entity_poses(entity_pos_dup, "end_pos.txt");
		dump_entity_move_info(move_diffs, "move_poses.txt");
		dump_aoi_mgrs(mgrs);
		return false;
	}
	return true;
}

void test_traceback()
{
	std::uint32_t max_entity_size = 2;
	float max_aoi_radius = 120.0f;
	float min_aoi_radius = 20.0f;
	pos_t border_min{ -1000.0f, -1000.0f, -1000.0f };
	pos_t border_max{ 1000.0f, 1000.0f, 1000.0f };
	std::vector<pos_t> entity_poses;
	std::vector<pos_unit_t> entity_radiuses;
	std::vector<std::pair<guid_t, pos_t>> entity_diffs;
	entity_poses.push_back(pos_t{ -651.189f, 0, -592.542f });
	entity_poses.push_back(pos_t{ -635.744f, 0, -540.063f });
	entity_radiuses.push_back(24);
	entity_radiuses.push_back(54);
	pos_t temp_pos;
	temp_pos[0] = -648.922f;
	temp_pos[1] = 0;
	temp_pos[2] = -592.094f;
	entity_diffs.emplace_back(0, temp_pos);
	auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
	test_move_speed(total_aoi_mgrs, entity_poses, entity_radiuses, entity_diffs);
	destroy_aoi_mgrs(total_aoi_mgrs);
}
int main()
{
	std::uint32_t max_entity_size = 1000;
	float max_aoi_radius = 120.0f;
	float min_aoi_radius = 20.0f;
	pos_t border_min{ -10000.0f, -10000.0f, -10000.0f };
	pos_t border_max{ 10000.0f, 10000.0f, 10000.0f };

	//std::vector<pos_t> entity_poses;
	//std::vector<pos_unit_t> entity_radius;
	//load_input(entity_poses, entity_radius);
	//auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
	//test_add(total_aoi_mgrs, entity_poses, entity_radius);
	//destroy_aoi_mgrs(total_aoi_mgrs);

	//for (std::size_t i = 0; i < 10; i++)
	//{
	//	std::vector<pos_t> entity_poses;
	//	std::vector<pos_unit_t> entity_radius;
	//	create_entity_poses(border_min, border_max, max_entity_size - 1, max_aoi_radius, entity_poses, entity_radius, 0.01);
	//	auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
	//	if (!test_trace(total_aoi_mgrs, entity_poses, entity_radius))
	//	{
	//		std::cout << "fail in test_trace" << std::endl;
	//		return false;
	//	}
	//	std::cout << "finish test_trace " << i << std::endl;
	//	destroy_aoi_mgrs(total_aoi_mgrs);
	//}

	
	for (std::size_t i = 0; i < 100; i++)
	{
		std::vector<pos_t> entity_poses;
		std::vector<pos_unit_t> entity_radius;
		create_entity_poses(border_min, border_max, max_entity_size - 1, max_aoi_radius, entity_poses, entity_radius);
		auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
		if (!test_add(total_aoi_mgrs, entity_poses, entity_radius))
		{
			std::cout << "fail in test_add" << std::endl;
			dump_input(entity_poses, entity_radius);
			dump_aoi_mgrs(total_aoi_mgrs);
			return false;
		}
		std::cout << "finish test_add " << i << std::endl;
		destroy_aoi_mgrs(total_aoi_mgrs);
	}

	//for (std::size_t i = 0; i < 10; i++)
	//{
	//	std::vector<pos_t> entity_poses;
	//	std::vector<pos_unit_t> entity_radius;
	//	create_entity_poses(border_min, border_max, max_entity_size - 1, max_aoi_radius, entity_poses, entity_radius);
	//	// load_input(entity_poses, entity_radius);
	//	auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
	//	if (!test_delete(total_aoi_mgrs, entity_poses, entity_radius))
	//	{
	//		std::cout << "fail in test_delete" << std::endl;
	//		dump_input(entity_poses, entity_radius);
	//		return false;
	//	}
	//	std::cout << "finish test_delete " << i << std::endl;
	//	destroy_aoi_mgrs(total_aoi_mgrs);
	//}


	//for (std::size_t i = 0; i < 100; i++)
	//{
	//	std::vector<pos_t> entity_poses;
	//	std::vector<pos_unit_t> entity_radius;
	//	create_entity_poses(border_min, border_max, max_entity_size - 1, max_aoi_radius, entity_poses, entity_radius);
	//	auto total_aoi_mgrs = create_aoi_mgrs(max_entity_size, max_aoi_radius, min_aoi_radius, border_min, border_max);
	//	auto move_diffs = create_entity_diffs(entity_poses, entity_radius, 0.1f, 1000);
	//	if (!test_move_speed(total_aoi_mgrs, entity_poses, entity_radius, move_diffs))
	//	{
	//		std::cout << "fail in test_move_speed" << std::endl;
	//		return false;
	//	}
	//	std::cout << "finish test_move_speed " << i << std::endl;
	//	destroy_aoi_mgrs(total_aoi_mgrs);
	//}
	// 
	//for (std::size_t i = 0; i < 10; i++)
	//{
	//	if (!test_trace())
	//	{
	//		std::cout << "failt in test trace" << std::endl;
	//		return 1;
	//	}
	//	std::cout << "finish test trace round " << i << std::endl;
	//}
	//for (int i = 0; i < 10; i++)
	//{
	//	if (!test_add())
	//	{
	//		std::cout << "failed in test add" << std::endl;
	//		return 1;
	//	}
	//}
}

