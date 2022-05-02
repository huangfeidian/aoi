#include "aoi_manager.h"
#include "aoi_interface.h"
#include <cmath>
// 判断是否在扇形范围内
using namespace spiritsaway::aoi;
#define PI 3.14159f
static bool is_in_fan(const pos_t& center, float yaw, float yaw_range, const pos_t& cur)
{
	if (yaw_range > PI / 2)
	{
		if (yaw <= PI)
		{
			return !is_in_fan(center, yaw + PI, PI - yaw_range, cur);
		}
		else
		{
			return !is_in_fan(center, yaw - PI, PI - yaw_range, cur);
		}
	}
	auto diff_x = cur[0] - center[0];
	auto diff_z = cur[2] - center[2];

	auto len = std::sqrt(diff_x * diff_x + diff_z * diff_z);

	auto cos_range = std::cos(yaw_range);
	auto sin_yaw = std::sin(yaw);
	auto cos_yaw = std::cos(yaw);

	auto area = (cos_yaw * diff_x + sin_yaw * diff_z) / len;
	return area >= cos_range;

}

static std::vector<guid_t> entity_vec_to_guid(const std::vector<aoi_entity*> entities)
{
	std::vector<guid_t> result;
	result.reserve(entities.size());
	for(auto one_entity: entities)
	{
		result.push_back(one_entity->guid());
	}
	return result;
}
aoi_manager::aoi_manager(aoi_interface* in_aoi_impl, aoi_idx_t in_max_entity_size, pos_unit_t in_max_aoi_radius, pos_t in_min, pos_t in_max)
: m_entities(in_max_entity_size + 1, nullptr)
, aoi_impl(in_aoi_impl)
, min(in_min)
, max(in_max)
, max_aoi_radius(in_max_aoi_radius)
{
	m_avail_slots = std::vector<aoi_idx_t>(in_max_entity_size);
	for (int i = 0; i < m_avail_slots.size(); i++)
	{
		m_avail_slots[i] = i + 1;
	}
	std::reverse(m_avail_slots.begin(), m_avail_slots.end());
}
aoi_manager::~aoi_manager()

{
	if (aoi_impl)
	{
		delete aoi_impl;
		aoi_impl = nullptr;
	}
	for (auto one_entity : m_entities)
	{
		if (one_entity)
		{
			delete one_entity;
		}
	}
	m_entities.clear();
}
bool aoi_manager::is_in_border(const pos_t& pos) const
{
	if(pos[0] <= min[0] || pos[0] >= max[0])
	{
		return false;
	}
	if(pos[1] <= min[1] || pos[1] >= max[1])
	{
		return false;
	}
	if(pos[2] <= min[2] || pos[2] >= max[2])
	{
		return false;
	}
	return true;
}
aoi_idx_t aoi_manager::add_entity(guid_t guid,  const aoi_controler& aoi_ctrl, const pos_t& pos, aoi_callback_t aoi_callback)
{
	if(!aoi_impl)
	{
		return 0;
	}
	if(aoi_ctrl.radius >= max_aoi_radius)
	{
		return 0;
	}
	if(!is_in_border(pos))
	{
		return 0;
	}
	auto cur_aoi_idx = request_entity_slot();
	if (cur_aoi_idx == 0)
	{
		return 0;
	}
	auto cur_entity = m_entities[cur_aoi_idx];

	cur_entity->activate(guid, aoi_ctrl, pos, aoi_callback);
	if (!aoi_impl->add_entity(cur_entity))
	{
		renounce_entity_idx(cur_aoi_idx);
		return 0;
	}
	else
	{
		return cur_aoi_idx;
	}
}

bool aoi_manager::remove_entity(aoi_idx_t aoi_idx)
{


	
	auto cur_entity = m_entities[aoi_idx];
	if (!cur_entity)
	{
		return false;
	}
	if(entities_removed.find(aoi_idx) != entities_removed.end())
	{
		return false;
	}
	aoi_impl->remove_entity(cur_entity);

	std::vector<aoi_idx_t> temp;
	temp = cur_entity->force_interest_in();
	for(auto one_aoi_idx: temp)
	{
		
		cur_entity->leave_by_force(*m_entities[one_aoi_idx]);
	}
	temp = cur_entity->force_interested_by();
	for(auto one_aoi_idx : temp)
	{
		m_entities[one_aoi_idx]->leave_by_force(*cur_entity);
	}

	temp = cur_entity->interest_in();
	for(auto one_aoi_idx : temp)
	{

		cur_entity->leave_impl(*m_entities[one_aoi_idx]);
	}
	temp = cur_entity->interested_by();
	for(auto one_aoi_idx : temp)
	{
		m_entities[one_aoi_idx]->leave_impl(*cur_entity);

	}
	cur_entity->deactivate();

	entities_removed.insert(cur_entity->aoi_idx);
	return true;

}

bool aoi_manager::change_entity_radius(aoi_idx_t aoi_idx, pos_unit_t radius)
{
	if(radius >= max_aoi_radius)
	{
		return false;
	}

	if (aoi_idx >= m_entities.size())
	{
		return false;
	}
	auto cur_entity = m_entities[aoi_idx];
	aoi_impl->on_radius_update(cur_entity, radius);
	return true;
}

bool aoi_manager::change_entity_pos(aoi_idx_t aoi_idx, pos_t pos)
{
	if(!is_in_border(pos))
	{
		return false;
	}
	if (aoi_idx >= m_entities.size())
	{
		return false;
	}
	auto cur_entity = m_entities[aoi_idx];
	aoi_impl->on_position_update(cur_entity, pos);
	return true;
}

bool aoi_manager::add_force_aoi(aoi_idx_t from, aoi_idx_t to)
{
	if (from >= m_entities.size() || to >= m_entities.size())
	{
		return false;
	}
	auto from_ent = m_entities[from];
	auto to_ent = m_entities[to];
	if (!from_ent || !to_ent)
	{
		return false;
	}
	return from_ent->enter_by_force(*to_ent);
}
bool aoi_manager::remove_force_aoi(aoi_idx_t from, aoi_idx_t to)
{
	if (from >= m_entities.size() || to >= m_entities.size())
	{
		return false;
	}
	auto from_ent = m_entities[from];
	auto to_ent = m_entities[to];
	if (!from_ent || !to_ent)
	{
		return false;
	}
	return from_ent->leave_by_force(*to_ent);
}
const std::vector<aoi_idx_t>& aoi_manager::interest_in(aoi_idx_t aoi_idx)const
{
	if (aoi_idx >= m_entities.size())
	{
		return m_invalid_result;
	}
	auto cur_entity = m_entities[aoi_idx];
	if (!cur_entity)
	{
		return m_invalid_result;
	}
	return cur_entity->interest_in();
}
const std::vector<aoi_idx_t>& aoi_manager::interested_by(aoi_idx_t aoi_idx)const
{
	if (aoi_idx >= m_entities.size())
	{
		return m_invalid_result;
	}
	auto cur_entity = m_entities[aoi_idx];
	if (!cur_entity)
	{
		return m_invalid_result;
	}
	return cur_entity->interested_by();
}

std::vector<guid_t> aoi_manager::entity_in_circle(pos_t center, pos_unit_t radius)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(aoi_impl->entity_in_circle(center, radius));
}

std::vector<guid_t> aoi_manager::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(aoi_impl->entity_in_cylinder(center, radius, height));
}

std::vector<guid_t> aoi_manager::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(aoi_impl->entity_in_rectangle(center, x_width, z_width));
}

std::vector<guid_t> aoi_manager::entity_in_square(pos_t center, pos_unit_t width)
{
	return entity_in_rectangle(center, width, width);
}

std::vector<guid_t> aoi_manager::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(aoi_impl->entity_in_cuboid(center, x_width, z_width, y_height));
}

std::vector<guid_t> aoi_manager::entity_in_cube(pos_t center, pos_unit_t width)
{
	return entity_vec_to_guid(aoi_impl->entity_in_cuboid(center, width, width, width));
}
std::vector<guid_t> aoi_manager::entity_in_fan(pos_t center, pos_unit_t radius, float yaw, float yaw_range)
{
	if (yaw < 0 || yaw >= 2 * PI)
	{
		return {};
	}
	if (yaw_range <= 0 || yaw_range >= PI)
	{
		return {};
	}
	auto circle_entities = aoi_impl->entity_in_circle(center, radius);
	std::vector<guid_t> result;
	result.reserve(circle_entities.size() / 2 + 1);
	for(auto one_ent: circle_entities)
	{

		if(!is_in_fan(center, yaw, yaw_range, one_ent->pos()))
		{
			continue;
		}
		result.push_back(one_ent->guid());

	}
	return result;
}

void aoi_manager::dump(std::ostream& out_debug) const
{
	for (auto entity : m_entities)
	{
		if (!entity)
		{
			continue;
		}
		auto guid = entity->guid();
		out_debug << "info for guid " << guid << " pos: " <<entity->pos()[0]<<","<<entity->pos()[2]<<" radius:"<<entity->aoi_ctrl().radius<< std::endl;
		std::vector<guid_t> temp;
		temp.insert(temp.end(), entity->interest_in().begin(), entity->interest_in().end());
		std::sort(temp.begin(), temp.end());
		out_debug << "interest in is:";
		for (auto one_guid : temp)
		{
			out_debug << one_guid << ", ";
		}
		out_debug << std::endl;

	}
	out_debug << "aoi_impl begin" << std::endl;
	if (aoi_impl)
	{
		aoi_impl->dump(out_debug);
	}
}

aoi_idx_t aoi_manager::request_entity_slot()
{
	if (m_avail_slots.empty())
	{
		return 0;
	}
	auto result = m_avail_slots.back();
	m_avail_slots.pop_back();

	auto cur_entity = m_entities[result];
	if (!cur_entity)
	{
		cur_entity = new aoi_entity(aoi_idx_t(result), aoi_idx_t(m_entities.size()));

		m_entities[result] = cur_entity;
	}
	return result;
}

void aoi_manager::renounce_entity_idx(aoi_idx_t aoi_idx)
{
	m_avail_slots.push_back(aoi_idx);
}

void aoi_manager::update()
{
	aoi_impl->update_all(m_entities);
	for (auto one_idx : entities_removed)
	{
		// 更新完成之后才释放期间remove的entity idx 避免一帧内出现添加后删除 或者删除后添加的情况
		renounce_entity_idx(one_idx);
	}
	entities_removed.clear();

}

const aoi_entity* aoi_manager::get_aoi_entity(aoi_idx_t aoi_idx) const
{
	if (aoi_idx >= m_entities.size())
	{
		return nullptr;
	}
	return m_entities[aoi_idx];
}