#include "aoi_entity.h"

using namespace spiritsaway::aoi;
bool aoi_radius_entity::pos_in_aoi_radius(const aoi_pos_entity& other) const
{
    const auto& self_pos = m_owner->pos();
    const auto& other_pos = other.pos();
    auto diff_x = self_pos[0] - other_pos[0];
    auto diff_y = self_pos[1] - other_pos[1];
    auto diff_z = self_pos[2] - other_pos[2];
    if((diff_x * diff_x + diff_z * diff_z) > m_aoi_radius_ctrl.radius * m_aoi_radius_ctrl.radius)
    {
        return false;
    }
    if(m_aoi_radius_ctrl.height > 0)
    {
        if(diff_y > m_aoi_radius_ctrl.height || diff_y < -m_aoi_radius_ctrl.height)
        {
            return false;
        }
    }
    return true;
}
bool aoi_radius_entity::can_pass_flag_check(const aoi_pos_entity& other) const
{
    // other能否进入当前entity的aoi 的flag测试
    if(other.guid() == m_owner->guid())
    {
        return false;
    }

    return m_aoi_radius_ctrl.interest_in_flag & other.interested_by_flag();// 如果两个flag取and 不为0 则代表可以进入aoi
}
bool aoi_radius_entity::can_add_enter(const aoi_pos_entity& other, bool ignore_dist, bool force_add) const
{
    if(!can_pass_flag_check(other))
    {
        return false;
    }
    if(!force_add  && m_interest_in.size() >= m_aoi_radius_ctrl.max_interest_in)
    {
        return false;
    }
    if (has_interest_in(other.pos_idx()))
    {
        return false;
    }
    if(!ignore_dist)
    {
        if(!pos_in_aoi_radius(other))
        {
            return false;
        }
    }
    return true;
}
bool aoi_radius_entity::enter_by_pos(aoi_pos_entity& other, bool ignore_dist)
{
    if(!can_add_enter(other, ignore_dist))
    {
        return false;
    }
    enter_impl(other);
    return true;
}
void aoi_radius_entity::enter_impl(aoi_pos_entity& other)
{		

    set_interest_in(other.pos_idx());
    other.set_interested_by(this->radius_idx());
    m_aoi_callback(this->radius_idx(), other.guid(), true);
}

bool aoi_radius_entity::enter_by_force(aoi_pos_entity& other)
{
    if (std::find(m_force_interest_in.begin(), m_force_interest_in.end(), other.pos_idx()) != m_force_interest_in.end())
    {
        return false;
    }
    if(!can_add_enter(other, true, true))
    {
        return false;
    }
    

    m_force_interest_in.push_back(other.pos_idx());
    other.add_force_interested_by(m_radius_idx);
    enter_impl(other);
    return true;
    
}
void aoi_radius_entity::leave_impl(aoi_pos_entity& other)
{
    remove_interest_in(other.pos_idx());
    other.remove_interested_by(this->m_radius_idx);
    m_aoi_callback(m_radius_idx, other.guid(), false);
}
bool aoi_radius_entity::leave_by_pos(aoi_pos_entity& other)
{
    if(!has_interest_in(other.pos_idx()))
    {
        return false;
    }
    if (std::find(m_force_interest_in.begin(), m_force_interest_in.end(), other.pos_idx()) != m_force_interest_in.end())
    {
        return false;
    }
    leave_impl(other);
    return true;
}
bool aoi_radius_entity::leave_by_force(aoi_pos_entity& other)
{
    if (!remove_in_vec(m_force_interest_in, other.pos_idx()))
    {
        return false;
    }
    
    if(!pos_in_aoi_radius(other))
    {
        leave_impl(other);
        return true;
    }
    else
    {
        return false;
    }
}

void aoi_radius_entity::activate(aoi_pos_entity* in_owner, const aoi_radius_controler& aoi_radius_ctrl, const aoi_callback_t aoi_cb)
{
    m_owner = in_owner;
    m_aoi_radius_ctrl = aoi_radius_ctrl;
    m_aoi_callback = aoi_cb;
    cacl_data = nullptr;
}

void aoi_radius_entity::deactivate()
{
    m_aoi_callback = nullptr;
    cacl_data = nullptr;
    m_owner = nullptr;
}
guid_t aoi_radius_entity::guid() const
{
    return m_owner->guid();
}

const pos_t& aoi_radius_entity::pos() const
{
    return m_owner->pos();
}



void aoi_radius_entity::update_by_pos(const std::vector<aoi_pos_entity*>& new_interest_in, const std::vector<aoi_pos_entity*>& all_entities, std::vector<std::uint8_t>& diff_vec)
{
    for (auto one_ent : new_interest_in)
    {
        diff_vec[one_ent->pos_idx().value] = 1;
    }
    for (auto one_ent_idx : m_interest_in)
    {
        if (!diff_vec[one_ent_idx.value])
        {
            leave_by_pos(*all_entities[one_ent_idx.value]);
        }
    }
    for (auto one_ent : new_interest_in)
    {
        enter_by_pos(*one_ent, true);
        diff_vec[one_ent->pos_idx().value] = 0;
    }
}

bool aoi_radius_entity::try_enter(aoi_pos_entity& other)
{
    return enter_by_pos(other);
}

bool aoi_radius_entity::try_leave(aoi_pos_entity& other)
{
    if (!pos_in_aoi_radius(other))
    {
        return leave_by_pos(other);
    }
    return false;
}

void aoi_pos_entity::activate(const pos_t& in_pos, std::uint64_t in_interested_by_flag, guid_t in_guid)
{
    m_pos = in_pos;
    m_guid = in_guid;
    m_interested_by_flag = in_interested_by_flag;
}

void aoi_pos_entity::add_radius_entity(aoi_radius_entity* in_radius_entity, const aoi_radius_controler& aoi_radius_ctrl, const aoi_callback_t aoi_cb)
{
    m_radius_entities.push_back(in_radius_entity);
    in_radius_entity->activate(this, aoi_radius_ctrl, aoi_cb);
    std::sort(m_radius_entities.begin(), m_radius_entities.end(), [](const aoi_radius_entity* a, const aoi_radius_entity* b)
        {
            return a->aoi_radius_ctrl().radius > b->aoi_radius_ctrl().radius;
        });
}
aoi_radius_entity* aoi_pos_entity::remove_radius_entity(aoi_radius_idx cur_radius_idx)
{
    for (std::uint32_t i = 0; i < m_radius_entities.size(); i++)
    {
        if (m_radius_entities[i]->radius_idx().value == cur_radius_idx.value)
        {
            auto result = m_radius_entities[i];
            m_radius_entities.erase(m_radius_entities.begin() + i);
            return result;
        }
    }
    return nullptr;
}

void aoi_pos_entity::deactivate()
{
    //std::vector<aoi_idx_t> temp;
    //temp = cur_entity->force_interest_in();
    //for (auto one_aoi_idx : temp)
    //{

    //    cur_entity->leave_by_force(*m_entities[one_aoi_idx]);
    //}
    //temp = cur_entity->force_interested_by();
    //for (auto one_aoi_idx : temp)
    //{
    //    m_entities[one_aoi_idx]->leave_by_force(*cur_entity);
    //}

    //temp = cur_entity->interest_in();
    //for (auto one_aoi_idx : temp)
    //{

    //    cur_entity->leave_impl(*m_entities[one_aoi_idx]);
    //}
    //temp = cur_entity->interested_by();
    //for (auto one_aoi_idx : temp)
    //{
    //    m_entities[one_aoi_idx]->leave_impl(*cur_entity);

    //}
}