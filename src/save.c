#include <sim.h>
#include <stdio.h>
#include <string.h>

#define SAVE_PATH "tavernsavefile.txt"

int save_game(const char* path,
              const World* w,
              const Tavern* b,
              const Merchant* m,
              const PeriodicPayment* p)
{
    FILE* f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "version=1\n\n");

    fprintf(f, "[world]\n");
    fprintf(f, "day=%d\n\n", w->day);

    fprintf(f, "[tavern]\n");
    fprintf(f, "money=%.2f\n", b->money);
    fprintf(f, "ale_price=%.2f\n", b->ale_price);
    fprintf(f, "wine_price=%.2f\n", b->wine_price);
    fprintf(f, "ale=%d\n", b->ale.amount);
    fprintf(f, "wine=%d\n", b->wine.amount);
    fprintf(f, "quality_actual=%.3f\n", b->quality_actual);
    fprintf(f, "quality_perceived=%.3f\n", b->quality_perceived);
    fprintf(f, "rumor=%.3f\n", b->rumor);
    fprintf(f, "consistency=%.3f\n", b->consistency);
    fprintf(f, "handsomeness=%.3f\n", b->handsomeness);
    fprintf(f, "reputation=%.3f\n\n", b->reputation);

    fprintf(f, "[merchant]\n");
    fprintf(f, "price_per_ale=%.3f\n", m->price_per_ale);
    fprintf(f, "price_per_wine=%.3f\n", m->price_per_wine);
    fprintf(f, "quality=%.3f\n", m->quality);
    fprintf(f, "instability=%.3f\n\n", m->instability);

    fprintf(f, "[payment]\n");
    fprintf(f, "pay_period=%d\n", p->pay_period);
    fprintf(f, "next_payment_day=%d\n", p->next_payment_day);
    fprintf(f, "rent_amount=%d\n", p->rent_amount);

    fclose(f);
    return 1;
}


int load_game(const char* path,
              World* w,
              Tavern* b,
              Merchant* m,
              PeriodicPayment* p)
{
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    /*
    why we do this before loading
    is because if we don't write
    0 to all; 
     * some fields might not appear
     * some values might fail to parse
     * some fields might be added later
    */     
    memset(w, 0, sizeof(*w));
    memset(b, 0, sizeof(*b));
    memset(m, 0, sizeof(*m));
    memset(p, 0, sizeof(*p));

    char line[256];
    enum { NONE, WORLD, TAVERN, MERCHANT, PAYMENT } section = NONE;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        if (strcmp(line, "[world]\n") == 0) {
            section = WORLD;
            continue;
        }
        if (strcmp(line, "[tavern]\n") == 0) {
            section = TAVERN;
            continue;
        }
        if (strcmp(line, "[merchant]\n") == 0) {
            section = MERCHANT;
            continue;
        }
        if (strcmp(line, "[payment]\n") == 0) {
            section = PAYMENT;
            continue;
        }

        switch (section) {
            case WORLD:
                sscanf(line, "day=%d", &w->day);
                break;

            case TAVERN:
                sscanf(line, "money=%f", &b->money);
                sscanf(line, "ale_price=%f", &b->ale_price);
                sscanf(line, "wine_price=%f", &b->wine_price);
                sscanf(line, "ale=%d", &b->ale.amount);
                sscanf(line, "wine=%d", &b->wine.amount);
                sscanf(line, "quality_actual=%f", &b->quality_actual);
                sscanf(line, "quality_perceived=%f", &b->quality_perceived);
                sscanf(line, "rumor=%f", &b->rumor);
                sscanf(line, "consistency=%f", &b->consistency);
                sscanf(line, "handsomeness=%f", &b->handsomeness);
                sscanf(line, "reputation=%f", &b->reputation);
                break;

            case MERCHANT:
                sscanf(line, "price_per_ale=%f", &m->price_per_ale);
                sscanf(line, "price_per_wine=%f", &m->price_per_wine);
                sscanf(line, "quality=%f", &m->quality);
                sscanf(line, "instability=%f", &m->instability);
                break;

            case PAYMENT:
                sscanf(line, "pay_period=%d", &p->pay_period);
                sscanf(line, "next_payment_day=%d", &p->next_payment_day);
                sscanf(line, "rent_amount=%d", &p->rent_amount);
                break;

            default:
                break;
        }
    }

    fclose(f);

    /* Re-link pointers */
    b->supplier = m;

    /* Defensive clamping */
    b->rumor = CLAMP(b->rumor, 0, 1);
    b->consistency = CLAMP(b->consistency, 0, 1);
    b->reputation = CLAMP(b->reputation, 0, 1);

    return 1;
}

