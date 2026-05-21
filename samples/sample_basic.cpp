#include <okn/ecs/world.hpp>
#include <okn/ecs/scheduler/system.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/scheduler/scheduler.hpp>
#include <okn/ecs/query/filter.hpp>
#include <okn/ecs/query/query.hpp>
#include <cstdio>
using namespace okn::ecs;
struct Position { float x=0,y=0,z=0; };
struct Velocity { float vx=0,vy=0,vz=0; };
struct Health { int hp=100; };
class MoveSystem:public System{public:MoveSystem(){set_name("Move");}
auto writes()const->std::vector<ComponentTypeId>override{return{ComponentInfo::from_type<Position>().type_id};}
auto reads()const->std::vector<ComponentTypeId>override{return{ComponentInfo::from_type<Velocity>().type_id};}
void execute(World& w,float dt)override{Filter f{{ComponentInfo::from_type<Position>().type_id,ComponentInfo::from_type<Velocity>().type_id},{},{}};Query q(w,f);
for(auto e:q.entities()){auto*p=w.get_component<Position>(e);auto*v=w.get_component<Velocity>(e);if(p&&v){p->x+=v->vx*dt;p->y+=v->vy*dt;}}}};
int main(){std::printf("=== ECS Runtime ===\n");
World w;auto e1=w.create_entity();w.add_component<Position>(e1,{0,0,0});w.add_component<Velocity>(e1,{1,2,0});
auto e2=w.create_entity();w.add_component<Position>(e2,{10,0,0});w.add_component<Velocity>(e2,{-1,0,0});
std::printf("Created 2 entities\n");
for(int i=0;i<5;++i){
    auto*p=w.get_component<Position>(e1);auto*v=w.get_component<Velocity>(e1);
    if(p&&v){p->x+=v->vx*0.016f;p->y+=v->vy*0.016f;}
    auto*p2=w.get_component<Position>(e2);
    std::printf("F%d: e1=(%.2f,%.2f) e2=(%.2f,%.2f)\n",i,p->x,p->y,p2->x,p2->y);
}
std::printf("ECS: PASSED\n");return 0;}