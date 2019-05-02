#include <iostream>

//游戏碰撞体类型
enum class GameObjectColliderType {
	None = 0, //关闭

	Circle = 1,  //严格圆
	OBB = 2,  //矩形
	Ellipse = 3,  //椭圆
	Diamond = 4,  //菱形
	Triangle = 5,  //三角
	Point = 6,  //点

	BentLazer = 9,//曲线激光
};

//游戏碰撞体
struct GameObjectCollider {
	GameObjectColliderType type;  //碰撞体类型
	float r;                      //半径，用于严格圆（未使用）
	float a;                      //椭圆半长轴、矩形半宽
	float b;                      //椭圆半短轴、矩形半高
	float rot;                    //相对旋转
	float dx;                     //相对偏移x
	float dy;                     //相对偏移y

	float circum_r;               //外接圆

	float absx;                   //计算后的绝对坐标x
	float absy;                   //计算后的绝对坐标y
	float absrot;                 //计算后的绝对旋转方向

	int id;
	GameObjectCollider* next;
	GameObjectCollider* last;

	//重置数值
	void reset() {
		type = GameObjectColliderType::Ellipse;
		r = 0.0f; a = 0.0f; b = 0.0f; rot = 0.0f;
		dx = 0.0f; dy = 0.0f;
		circum_r = 0.0f;
		absx = 0.0f; absy = 0.0f; absrot = 0.0f;
		id = 0;
		last = nullptr; next = nullptr;
	}
	//计算外接圆和对应的XMath库碰撞体类型
	void calcircum() {
		switch (type)
		{
		case GameObjectColliderType::Circle:
			circum_r = a > b ? a : b;
			break;
		case GameObjectColliderType::OBB:
			circum_r = std::sqrtf(std::powf(a, 2.0f) + std::powf(b, 2.0f));
			break;
		case GameObjectColliderType::Ellipse:
			circum_r = a > b ? a : b;
			break;
		case GameObjectColliderType::Diamond:
			circum_r = a > b ? a : b;
			break;
		case GameObjectColliderType::Triangle:
			circum_r = std::sqrtf(std::powf(a, 2.0f) + std::powf(b, 2.0f));
			break;
		case GameObjectColliderType::Point:
			circum_r = 0.0f;
			break;
		}
	}
	//根据偏移计算绝对坐标和旋转
	void caloffset(float x, float y, float _rot) {
		//可能是计算方法有误
		absx = x + dx * std::cosf(-_rot) + dy * std::sinf(-_rot);
		absy = y + dy * std::cosf(-_rot) - dx * std::sinf(-_rot);
		absrot = _rot + rot;
	}
};

GameObjectCollider* collider = nullptr;

void Reset() {
	if (collider == nullptr) {
		collider = new GameObjectCollider();
	}
	else {
		if (collider->next != nullptr) {
			GameObjectCollider* ptr = collider->next;
			GameObjectCollider* pos = nullptr;
			while (ptr != nullptr) {
				pos = ptr->next;
				delete ptr; ptr = nullptr;
				ptr = pos; pos = nullptr;
			}
		}
	}
	collider->reset();
}

void Clear() {
	GameObjectCollider* ptr = collider;
	GameObjectCollider* pos = nullptr;
	while (ptr != nullptr) {
		pos = ptr->next;
		delete ptr; ptr = nullptr;
		ptr = pos; pos = nullptr;
	}
	collider = nullptr;
}

int main() {
	
	Reset();

	Reset();

	Clear();

	Clear();

	system("pause");
	return 0;
}
