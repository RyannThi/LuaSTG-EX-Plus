#include <iostream>

//��Ϸ��ײ������
enum class GameObjectColliderType {
	None = 0, //�ر�

	Circle = 1,  //�ϸ�Բ
	OBB = 2,  //����
	Ellipse = 3,  //��Բ
	Diamond = 4,  //����
	Triangle = 5,  //����
	Point = 6,  //��

	BentLazer = 9,//���߼���
};

//��Ϸ��ײ��
struct GameObjectCollider {
	GameObjectColliderType type;  //��ײ������
	float r;                      //�뾶�������ϸ�Բ��δʹ�ã�
	float a;                      //��Բ�볤�ᡢ���ΰ��
	float b;                      //��Բ����ᡢ���ΰ��
	float rot;                    //�����ת
	float dx;                     //���ƫ��x
	float dy;                     //���ƫ��y

	float circum_r;               //���Բ

	float absx;                   //�����ľ�������x
	float absy;                   //�����ľ�������y
	float absrot;                 //�����ľ�����ת����

	int id;
	GameObjectCollider* next;
	GameObjectCollider* last;

	//������ֵ
	void reset() {
		type = GameObjectColliderType::Ellipse;
		r = 0.0f; a = 0.0f; b = 0.0f; rot = 0.0f;
		dx = 0.0f; dy = 0.0f;
		circum_r = 0.0f;
		absx = 0.0f; absy = 0.0f; absrot = 0.0f;
		id = 0;
		last = nullptr; next = nullptr;
	}
	//�������Բ�Ͷ�Ӧ��XMath����ײ������
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
	//����ƫ�Ƽ�������������ת
	void caloffset(float x, float y, float _rot) {
		//�����Ǽ��㷽������
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
