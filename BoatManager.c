#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define NAME_LEN 128
#define LICENSE_LEN 16

typedef enum {
    slip,
    land,
    trailor,
    storage,
    no_place
} PlaceType;

typedef union {
    int slip_number;
    char bay_letter;
    char license_tag[LICENSE_LEN];
    int storage_space;
} LocationInfo;

typedef struct {
    char name[NAME_LEN];
    float length;
    PlaceType type;
    LocationInfo info;
    float amount_owed;
} Boat;

Boat* boats[MAX_BOATS];
int boat_count = 0;

PlaceType StringToPlaceType(char *PlaceString) {
    if (!strcasecmp(PlaceString, "slip")) return slip;
    if (!strcasecmp(PlaceString, "land")) return land;
    if (!strcasecmp(PlaceString, "trailor")) return trailor;
    if (!strcasecmp(PlaceString, "storage")) return storage;
    return no_place;
}

char *PlaceToString(PlaceType Place) {
    switch (Place) {
        case slip: return "slip";
        case land: return "land";
        case trailor: return "trailor";
        case storage: return "storage";
        case no_place: return "no_place";
        default:
            printf("How the faaark did I get here?\n");
            exit(EXIT_FAILURE);
    }
}

float rate(PlaceType type) {
    switch (type) {
        case slip: return 12.5;
        case land: return 14.0;
        case trailor: return 25.0;
        case storage: return 11.2;
        default: return 0.0;
    }
}

int compare_boats(const void *a, const void *b) {
    const Boat *ba = *(const Boat **)a;
    const Boat *bb = *(const Boat **)b;
    return strcasecmp(ba->name, bb->name);
}

int read_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    char line[256];
    while (fgets(line, sizeof(line), file) && boat_count < MAX_BOATS) {
        Boat* b = malloc(sizeof(Boat));
        if (!b) continue;

        char type_str[16], extra[LICENSE_LEN];
        sscanf(line, "%127[^,],%f,%15[^,],%15[^,],%f", b->name, &b->length, type_str, extra, &b->amount_owed);
        b->type = StringToPlaceType(type_str);
        switch (b->type) {
            case slip: b->info.slip_number = atoi(extra); break;
            case land: b->info.bay_letter = extra[0]; break;
            case trailor: strncpy(b->info.license_tag, extra, LICENSE_LEN); break;
            case storage: b->info.storage_space = atoi(extra); break;
            default: break;
        }
        boats[boat_count++] = b;
    }

    fclose(file);
    return 1;
}

void save_csv(const char* filename) {
    FILE* file = fopen(filename, "w");
    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        fprintf(file, "%s,%.0f,%s,", b->name, b->length, PlaceToString(b->type));
        switch (b->type) {
            case slip: fprintf(file, "%d,", b->info.slip_number); break;
            case land: fprintf(file, "%c,", b->info.bay_letter); break;
            case trailor: fprintf(file, "%s,", b->info.license_tag); break;
            case storage: fprintf(file, "%d,", b->info.storage_space); break;
            default: break;
        }
        fprintf(file, "%.2f\n", b->amount_owed);
    }
    fclose(file);
}

void print_inventory() {
    qsort(boats, boat_count, sizeof(Boat *), compare_boats);
    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        printf("%-20s %4.0f' ", b->name, b->length);
        switch (b->type) {
            case slip: printf("slip   # %2d", b->info.slip_number); break;
            case land: printf("land      %c", b->info.bay_letter); break;
            case trailor: printf("trailor %-6s", b->info.license_tag); break;
            case storage: printf("storage # %2d", b->info.storage_space); break;
            default: printf("unknown"); break;
        }
        printf("   Owes $%7.2f\n", b->amount_owed);
    }
}

int find_boat(const char* name) {
    for (int i = 0; i < boat_count; i++)
        if (strcasecmp(name, boats[i]->name) == 0)
            return i;
    return -1;
}

void add_boat(const char* csv) {
    if (boat_count >= MAX_BOATS) return;
    Boat* b = malloc(sizeof(Boat));
    if (!b) return;

    char type_str[16], extra[LICENSE_LEN];
    sscanf(csv, "%127[^,],%f,%15[^,],%15[^,],%f", b->name, &b->length, type_str, extra, &b->amount_owed);
    b->type = StringToPlaceType(type_str);
    switch (b->type) {
        case slip: b->info.slip_number = atoi(extra); break;
        case land: b->info.bay_letter = extra[0]; break;
        case trailor: strncpy(b->info.license_tag, extra, LICENSE_LEN); break;
        case storage: b->info.storage_space = atoi(extra); break;
        default: break;
    }

    boats[boat_count++] = b;
}

void remove_boat(const char* name) {
    int index = find_boat(name);
    if (index < 0) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[index]);
    for (int i = index; i < boat_count - 1; i++) boats[i] = boats[i + 1];
    boat_count--;
}

void payment(const char* name, float amount) {
    int index = find_boat(name);
    if (index < 0) {
        printf("No boat with that name\n");
        return;
    }
    if (amount > boats[index]->amount_owed) {
        printf("That is more than the amount owed, $%.2f\n", boats[index]->amount_owed);
    } else {
        boats[index]->amount_owed -= amount;
    }
}

void update_month() {
    for (int i = 0; i < boat_count; i++) {
        boats[i]->amount_owed += boats[i]->length * rate(boats[i]->type);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide the CSV file name\n");
        return 1;
    }

    if (!read_csv(argv[1])) {
        printf("Error reading file\n");
        return 1;
    }

    printf("Welcome to the Boat Management System\n");

    while (1) {
        char choice, name[NAME_LEN], csv[256];
        float amount;

        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        scanf(" %c", &choice);
        choice = tolower(choice);

        if (choice == 'i') print_inventory();
        else if (choice == 'a') {
            printf("Please enter the boat data in CSV format : ");
            scanf(" %[^\n]", csv);
            add_boat(csv);
        } else if (choice == 'r') {
            printf("Please enter the boat name : ");
            scanf(" %[^\n]", name);
            remove_boat(name);
        } else if (choice == 'p') {
            printf("Please enter the boat name : ");
            scanf(" %[^\n]", name);
            printf("Please enter the amount to be paid : ");
            scanf("%f", &amount);
            payment(name, amount);
        } else if (choice == 'm') update_month();
        else if (choice == 'x') break;
        else printf("Invalid option %c\n", choice);
    }

    save_csv(argv[1]);
    for (int i = 0; i < boat_count; i++) free(boats[i]);
    printf("Exiting the Boat Management System\n");
    return 0;
}
