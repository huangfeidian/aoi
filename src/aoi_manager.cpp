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

static std::vector<guid_t> entity_vec_to_guid(const std::vector<aoi_pos_entity*> entities)
{
	std::vector<guid_t> result;
	result.reserve(entities.size());
	for(auto one_entity: entities)
	{
		result.push_back(one_entity->guid());
	}
	return result;
}
aoi_manager::aoi_manager(bool is_radius_circle, aoi_interface* in_aoi_impl, aoi_idx_t in_max_entity_size, aoi_idx_t in_max_radius_size,  pos_t in_min_pos, pos_t in_max_pos, std::function<void(guid_t, const std::vector<aoi_notify_info>&)> in_aoi_cb)
: m_pos_entities(in_max_entity_size , nullptr)
, m_radius_entities(in_max_radius_size, nullptr)
, m_aoi_impl(in_aoi_impl)
, m_min_pos(in_min_pos)
, m_max_pos(in_max_pos)
, m_aoi_cb(in_aoi_cb)
, m_is_radius_circle(is_radius_circle)
{
	m_avail_pos_slots = std::vector<aoi_pos_idx>(in_max_entity_size-1);
	for (aoi_idx_t i = 0; i < m_avail_pos_slots.size(); i++)
	{
		m_avail_pos_slots[i] = aoi_pos_idx{ aoi_idx_t(i + 1 )};
	}
	std::reverse(m_avail_pos_slots.begin(), m_avail_pos_slots.end());

	m_avail_radius_slots = std::vector<aoi_radius_idx>(in_max_radius_size-1);
	for (aoi_idx_t i = 0; i < m_avail_radius_slots.size(); i++)
	{
		m_avail_radius_slots[i] = aoi_radius_idx{ aoi_idx_t(i + 1) };
	}
	std::reverse(m_avail_radius_slots.begin(), m_avail_radius_slots.end());
}
aoi_manager::~aoi_manager()

{
	if (m_aoi_impl)
	{
		delete m_aoi_impl;
		m_aoi_impl = nullptr;
	}
	for (auto one_entity : m_pos_entities)
	{
		if (one_entity)
		{
			delete one_entity;
		}
	}
	m_pos_entities.clear();

	for (auto one_entity : m_radius_entities)
	{
		if (one_entity)
		{
			delete one_entity;
		}
	}
	m_radius_entities.clear();
}
bool aoi_manager::is_in_border(const pos_t& pos) const
{
	if(pos[0] <= m_min_pos[0] || pos[0] >= m_max_pos[0])
	{
		return false;
	}
	if(pos[1] <= m_min_pos[1] || pos[1] >= m_max_pos[1])
	{
		return false;
	}
	if(pos[2] <= m_min_pos[2] || pos[2] >= m_max_pos[2])
	{
		return false;
	}
	return true;
}

aoi_pos_idx aoi_manager::add_pos_entity(guid_t guid, const pos_t& in_pos, std::uint64_t in_interested_by_flag)
{
	if (!m_aoi_impl || !is_in_border(in_pos))
	{
		return aoi_pos_idx{ 0 };
	}
	auto cur_aoi_pos_idx = request_pos_entity_slot();
	if (cur_aoi_pos_idx.value == 0)
	{
		return cur_aoi_pos_idx;
	}
	auto cur_pos_entity = m_pos_entities[cur_aoi_pos_idx.value];
	m_active_pos_entities.push_back(cur_pos_entity);
	cur_pos_entity->activate(in_pos, guid, in_interested_by_flag);
	if (!m_aoi_impl->add_pos_entity(cur_pos_entity))
	{
		renounce_pos_entity_idx(cur_aoi_pos_idx);
		return aoi_pos_idx{ 0 };
	}
	else
	{
		return cur_aoi_pos_idx;
	}

}
aoi_radius_idx aoi_manager::add_radius_entity(aoi_pos_idx in_pos_idx, const aoi_radius_controler& aoi_radius_ctrl)
{
	if(!m_aoi_impl  || !in_pos_idx.value || in_pos_idx.value >= m_pos_entities.size())
	{
		return aoi_radius_idx{ 0 };
	}
	
	auto cur_owner_entity = m_pos_entities[in_pos_idx.value];
	if (!cur_owner_entity)
	{
		return aoi_radius_idx{ 0 };
	}
	auto cur_aoi_radius_idx = request_radius_entity_slot();
	if (cur_aoi_radius_idx.value == 0)
	{
		return aoi_radius_idx{ 0 };
	}
	auto cur_radius_entity = m_radius_entities[cur_aoi_radius_idx.value];

	cur_owner_entity->add_radius_entity(cur_radius_entity, aoi_radius_ctrl);

	if (!m_aoi_impl->add_radius_entity(cur_radius_entity))
	{
		cur_owner_entity->remove_radius_entity(cur_aoi_radius_idx);
		renounce_radius_entity_idx(cur_aoi_radius_idx);
		return aoi_radius_idx{ 0 };
	}
	else
	{
		return cur_aoi_radius_idx;
	}
}

bool aoi_manager::remove_pos_entity(aoi_pos_idx pos_idx)
{


	if (pos_idx.value >= m_pos_entities.size())
	{
		return false;
	}
	auto cur_entity = m_pos_entities[pos_idx.value];
	if (!cur_entity)
	{
		return false;
	}
	if(m_pos_entities_removed.find(pos_idx.value) != m_pos_entities_removed.end())
	{
		return false;
	}
	if (!cur_entity->radius_entities().empty())
	{
		std::vector<aoi_radius_idx> temp_radius_idxes;
		temp_radius_idxes.reserve(cur_entity->radius_entities().size());
		for (const auto& one_radius_entity : cur_entity->radius_entities())
		{
			temp_radius_idxes.push_back(one_radius_entity->radius_idx());
		}
		for (auto one_radius_idx : temp_radius_idxes)
		{
			remove_radius_entity(one_radius_idx);
		}
	}
	
	m_aoi_impl->remove_pos_entity(cur_entity);

	for (std::uint32_t i = 0; i < m_active_pos_entities.size(); i++)
	{
		if (m_active_pos_entities[i] == cur_entity)
		{
			if ((i + 1) != m_active_pos_entities.size())
			{
				std::swap(m_active_pos_entities.back(), m_active_pos_entities[i]);
			}
			m_active_pos_entities.pop_back();
			break;
		}
	}
	cur_entity->invoke_aoi_cb(m_aoi_cb);

	cur_entity->deactivate();

	m_pos_entities_removed.insert(pos_idx.value);
	return true;

}

bool aoi_manager::remove_radius_entity(aoi_radius_idx radius_idx)
{


	if (radius_idx.value >= m_radius_entities.size())
	{
		return false;
	}
	auto cur_entity = m_radius_entities[radius_idx.value];
	if (!cur_entity)
	{
		return false;
	}
	if (m_radius_entities_removed.find(radius_idx.value) != m_radius_entities_removed.end())
	{
		return false;
	}
	m_aoi_impl->remove_radius_entity(cur_entity);

	cur_entity->owner().remove_radius_entity(radius_idx);
	cur_entity->deactivate();

	m_radius_entities_removed.insert(radius_idx.value);
	cur_entity->owner().invoke_aoi_cb(m_aoi_cb);

	return true;

}


bool aoi_manager::change_entity_radius(aoi_radius_idx radius_idx, pos_unit_t radius)
{


	if (radius_idx.value >= m_radius_entities.size())
	{
		return false;
	}
	auto cur_entity = m_radius_entities[radius_idx.value];
	if (!cur_entity)
	{
		return false;
	}
	m_aoi_impl->on_radius_update(cur_entity, radius);
	return true;
}

bool aoi_manager::change_entity_pos(aoi_pos_idx pos_idx, pos_t pos)
{
	if(!is_in_border(pos))
	{
		return false;
	}
	if (pos_idx.value >= m_pos_entities.size())
	{
		return false;
	}
	auto cur_entity = m_pos_entities[pos_idx.value];
	if (!cur_entity)
	{
		return false;
	}
	m_aoi_impl->on_position_update(cur_entity, pos);
	return true;
}

bool aoi_manager::add_force_aoi(aoi_pos_idx from, aoi_radius_idx to)
{
	if (from.value >= m_pos_entities.size() || to.value>= m_radius_entities.size())
	{
		return false;
	}
	auto from_ent = m_pos_entities[from.value];
	auto to_ent = m_radius_entities[to.value];
	if (!from_ent || !to_ent)
	{
		return false;
	}
	return to_ent->enter_by_force(*from_ent);
}
bool aoi_manager::remove_force_aoi(aoi_pos_idx from, aoi_radius_idx to)
{
	if (from.value >= m_pos_entities.size() || to.value >= m_radius_entities.size())
	{
		return false;
	}
	auto from_ent = m_pos_entities[from.value];
	auto to_ent = m_radius_entities[to.value];
	if (!from_ent || !to_ent)
	{
		return false;
	}
	return to_ent->leave_by_force(*from_ent);
}
const std::unordered_map<aoi_pos_idx, aoi_pos_entity*>& aoi_manager::interest_in(aoi_radius_idx radius_idx)const
{
	if (radius_idx.value >= m_radius_entities.size())
	{
		return m_invalid_pos_result;
	}
	auto cur_entity = m_radius_entities[radius_idx.value];
	if (!cur_entity)
	{
		return m_invalid_pos_result;
	}
	return cur_entity->interest_in();
}
std::vector<guid_t> aoi_manager::interest_in_guids(aoi_radius_idx radius_idx) const
{
	if (radius_idx.value >= m_radius_entities.size())
	{
		return {};
	}
	auto cur_entity = m_radius_entities[radius_idx.value];
	if (!cur_entity)
	{
		return {};
	}
	auto& pre_aoi_idxes = cur_entity->interest_in();
	std::vector<guid_t> result;
	result.reserve(pre_aoi_idxes.size());
	for(auto one_pair: pre_aoi_idxes)
	{
		result.push_back(one_pair.second->guid());
	}
	return result;
}
const std::unordered_set<aoi_radius_idx>& aoi_manager::interested_by(aoi_pos_idx pos_idx)const
{
	if (pos_idx.value >= m_pos_entities.size())
	{
		return m_invalid_radius_result;
	}
	auto cur_entity = m_pos_entities[pos_idx.value];
	if (!cur_entity)
	{
		return m_invalid_radius_result;
	}
	return cur_entity->interested_by();
}

std::vector<guid_t> aoi_manager::entity_in_circle(pos_t center, pos_unit_t radius)
{
	if(!m_aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(m_aoi_impl->entity_in_circle(center, radius));
}

std::vector<guid_t> aoi_manager::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)
{
	if(!m_aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(m_aoi_impl->entity_in_cylinder(center, radius, height));
}

std::vector<guid_t> aoi_manager::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)
{
	if(!m_aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(m_aoi_impl->entity_in_rectangle(center, x_width, z_width));
}

std::vector<guid_t> aoi_manager::entity_in_square(pos_t center, pos_unit_t width)
{
	return entity_in_rectangle(center, width, width);
}

std::vector<guid_t> aoi_manager::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)
{
	if(!m_aoi_impl)
	{
		return {};
	}
	return entity_vec_to_guid(m_aoi_impl->entity_in_cuboid(center, x_width, z_width, y_height));
}

std::vector<guid_t> aoi_manager::entity_in_cube(pos_t center, pos_unit_t width)
{
	return entity_vec_to_guid(m_aoi_impl->entity_in_cuboid(center, width, width, width));
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
	auto circle_entities = m_aoi_impl->entity_in_circle(center, radius);
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
	for (auto entity : m_radius_entities)
	{
		if (!entity)
		{
			continue;
		}
		auto guid = entity->guid();
		out_debug << "info for guid " << guid <<" radius id "<<entity->radius_idx().value<< " pos: " <<entity->pos()[0]<<","<<entity->pos()[2]<<" radius:"<<entity->aoi_radius_ctrl().radius<< std::endl;
		std::vector<guid_t> temp;
		temp.reserve(entity->interest_in().size());
		for (auto one_pair : entity->interest_in())
		{
			temp.push_back(one_pair.second->guid());
		}
		std::sort(temp.begin(), temp.end());
		out_debug << "interest in is:";
		for (auto one_guid : temp)
		{
			out_debug << one_guid << ", ";
		}
		out_debug << std::endl;

	}
	out_debug << "aoi_impl begin" << std::endl;
	if (m_aoi_impl)
	{
		m_aoi_impl->dump(out_debug);
	}
}

aoi_pos_idx aoi_manager::request_pos_entity_slot()
{
	if (m_avail_pos_slots.empty())
	{
		return aoi_pos_idx{ 0 };
	}
	auto result = m_avail_pos_slots.back();
	m_avail_pos_slots.pop_back();

	auto cur_entity = m_pos_entities[result.value];
	if (!cur_entity)
	{
		cur_entity = new aoi_pos_entity(aoi_pos_idx(result), aoi_idx_t(m_radius_entities.size()));

		m_pos_entities[result.value] = cur_entity;
	}
	return result;
}

void aoi_manager::renounce_pos_entity_idx(aoi_pos_idx pos_idx)
{
	m_avail_pos_slots.push_back(pos_idx);
}

aoi_radius_idx aoi_manager::request_radius_entity_slot()
{
	if (m_avail_radius_slots.empty())
	{
		return aoi_radius_idx{ 0 };
	}
	auto result = m_avail_radius_slots.back();
	m_avail_radius_slots.pop_back();

	auto cur_entity = m_radius_entities[result.value];
	if (!cur_entity)
	{
		cur_entity = new aoi_radius_entity(m_is_radius_circle, aoi_radius_idx(result), aoi_idx_t(m_pos_entities.size()));

		m_radius_entities[result.value] = cur_entity;
	}
	return result;
}

void aoi_manager::renounce_radius_entity_idx(aoi_radius_idx pos_idx)
{
	m_avail_radius_slots.push_back(pos_idx);
}

void aoi_manager::update()
{
	m_aoi_impl->update_all();
	// 计算好新的info之后 才能触发callback
	m_update_pos_entities = m_active_pos_entities;
	for (auto one_pos_entity : m_update_pos_entities)
	{
		one_pos_entity->invoke_aoi_cb(m_aoi_cb);

	}
	for (auto one_idx : m_pos_entities_removed)
	{
		// 更新完成之后才释放期间remove的entity idx 避免一帧内出现添加后删除 或者删除后添加的情况
		renounce_pos_entity_idx(aoi_pos_idx{ one_idx });
	}
	m_pos_entities_removed.clear();

	for (auto one_idx : m_radius_entities_removed)
	{
		// 更新完成之后才释放期间remove的entity idx 避免一帧内出现添加后删除 或者删除后添加的情况
		renounce_radius_entity_idx(aoi_radius_idx{ one_idx });
	}
	m_radius_entities_removed.clear();

}

