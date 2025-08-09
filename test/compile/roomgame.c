#include "../../include/stdarg.h"
#include "../../include/stdio.h"
#include "../../include/string.h"

#define MAX_INPUT_LENGTH 100
#define MAX_ROOMS 10

// ����ṹ��
typedef struct {
    char description[256];
    int north;
    int south;
    int east;
    int west;
} Room;

// ��Ϸ״̬
typedef struct {
    int currentRoom;
    bool gameOver;
} GameState;

// ��ʼ������
void initRooms(Room rooms[], int numRooms) {
    // �����ʼ�����䣬���磺
    sprintf(&rooms[0].description, "You are in a dark room with no doors.");
    rooms[0].north = 1;
    rooms[0].south = -1;
    rooms[0].east = -1;
    rooms[0].west = -1;
    // ... ��ʼ����������
}

// ��ʾ��������
void describeRoom(Room room) {
    printf("%s\n", room.description);
}

// �ƶ�����һ������
void move(int direction, Room rooms[], GameState *state) {
    int nextRoom = state->currentRoom + direction;
    if (nextRoom >= 0 && nextRoom < MAX_ROOMS && rooms[nextRoom].description[0] != '\0') {
        state->currentRoom = nextRoom;
        describeRoom(rooms[state->currentRoom]);
    } else {
        printf("You can't go that way.\n");
    }
}

// ����Ϸѭ��
void playGame(Room rooms[], GameState state) {
    char input[MAX_INPUT_LENGTH];
    while (!state.gameOver) {
        printf("Enter a command (N, S, E, W, or Q to quit): ");
        fgets(input, MAX_INPUT_LENGTH, stdin);
        input[strcspn(input, "\n")] = 0; // �Ƴ����з�

        if (strcmp(input, "N") == 0) {
            move(-1, rooms, &state);
        } else if (strcmp(input, "S") == 0) {
            move(1, rooms, &state);
        } // ... �������������ж�
        else if (strcmp(input, "Q") == 0) {
            state.gameOver = true;
            printf("Thanks for playing!\n");
        }
    }
}

int main() {
    Room rooms[MAX_ROOMS];
    GameState state = {0, false};
    initRooms(rooms, MAX_ROOMS);
    playGame(rooms, state);
    return 0;
}
