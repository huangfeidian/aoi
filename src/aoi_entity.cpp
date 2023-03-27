#include "aoi_entity.h"

using namespace spiritsaway::aoi;
bool aoi_radius_entity::pos_in_aoi_radius(const aoi_pos_entity& other) const
{
    const auto& self_pos = m_owner->pos();
    const auto& other_pos = other.pos();
    auto diff_x = self_pos[0] - other_pos[0];
    auto diff_y = (self_pos[1] - other_pos[1]) * -1;
    auto diff_z = self_pos[2] - other_pos[2];
    if((diff_x * diff_x + diff_z * diff_z) > m_aoi_radius_ctrl.radius * m_aoi_radius_ctrl.radius)
    {
        return false;
    }
    return check_height(diff_y);
}
bool aoi_radius_entity::check_height(pos_unit_t diff_y) const
{
    if (m_aoi_radius_ctrl.height > 0)
    {
        if (diff_y > m_aoi_radius_ctrl.height || diff_y < -m_aoi_radius_ctrl.height)
        {
            return false;
        }
    }
    return true;
}
bool aoi_radius_entity::check_flag(const aoi_pos_entity& other) const
{
    return m_aoi_radius_ctrl.interest_in_flag & other.interested_by_flag();// 如果两个flag取and 不为0 则代表可以进入aoi
}
bool aoi_radius_entity::can_add_enter(const aoi_pos_entity& other, bool ignore_dist, bool force_add) const
{
    if(!check_flag(other))
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

    set_interest_in(&other);
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
    

    m_force_interest_in[other.pos_idx()] = &other;
    other.add_force_interested_by(m_radius_idx);
    enter_impl(other);
    return true;
    
}
void aoi_radius_entity::leave_impl(aoi_pos_entity& other)
{
    remove_interest_in(&other);
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

    if (m_force_interest_in.erase(other.pos_idx()) == 0)
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

void aoi_pos_entity::check_add(aoi_pos_entity* other)
{
    if (guid() == other->guid())
    {
        return;
    }
    
    auto diff_x = m_pos[0] - other->pos()[0];
    auto diff_y = (m_pos[1] - other->pos()[1]) * -1;
    auto diff_z = m_pos[2] - other->pos()[2];
    auto diff_radius = std::sqrt(diff_x * diff_x + diff_z * diff_z);
    for (auto one_radius_entity : m_radius_entities)
    {

        if (!one_radius_entity->aoi_radius_ctrl().radius < diff_radius)
        {
            break;
        }
        if (!one_radius_entity->check_height(diff_y))
        {
            continue;
        }
        one_radius_entity->enter_by_pos(*other, true);


    }
    
}
void aoi_pos_entity::check_remove()
{


    for (auto one_radius_entity : m_radius_entities)
    {
        
        auto temp_iter = one_radius_entity->m_interest_in.begin();
        while (temp_iter != one_radius_entity->m_interest_in.end())
        {
            auto other = temp_iter->second;
            bool should_leave = false;
            do 
            {
                if (one_radius_entity->m_force_interest_in.find(temp_iter->first) != one_radius_entity->m_force_interest_in.end())
                {
                    break;
                }
                auto diff_x = m_pos[0] - other->pos()[0];
                auto diff_y = (m_pos[1] - other->pos()[1]) * -1;
                auto diff_z = m_pos[2] - other->pos()[2];
                auto diff_radius = std::sqrt(diff_x * diff_x + diff_z * diff_z);
                if (!one_radius_entity->aoi_radius_ctrl().radius < diff_radius)
                {
                    should_leave = true;
                    break;
                }
                if (!one_radius_entity->check_height(diff_y))
                {
                    should_leave = true;
                    break;
                }
                if (!one_radius_entity->check_flag(*other))
                {
                    should_leave = true;
                    break;
                }
            } while (0);
            if (should_leave)
            {
                temp_iter++;
                one_radius_entity->leave_by_pos(*other);
            }
            else
            {
                temp_iter++;
            }

        }
    }
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

void aoi_pos_entity::add_force_interested_by(aoi_radius_idx radius_idx)
{
    auto byte_offset = radius_idx.value / 8;
    auto bit_offset = radius_idx.value % 8;
    force_interested_by_bitset[byte_offset] |= (1 << bit_offset);
    m_force_interested_by.insert(radius_idx);
}
void aoi_pos_entity::remove_force_interested_by(aoi_radius_idx radius_idx)
{
    auto byte_offset = radius_idx.value / 8;
    auto bit_offset = radius_idx.value % 8;
    force_interested_by_bitset[byte_offset] ^= (1 << bit_offset);
    m_force_interested_by.erase(radius_idx);
}

pos_unit_t aoi_pos_entity::max_radius() const
{
    if (m_radius_entities.empty())
    {
        return 0;
    }
    return m_radius_entities[0]->aoi_radius_ctrl().radius;
}
