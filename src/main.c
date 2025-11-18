#include "difftree.h"
#include "optutils.h"
#include "memutils.h"
#include "utils.h"
#include "logutils.h"
#include "ioutils.h"

#define LOG_CATEGORY_OPT "OPTIONS"
#define LOG_CATEGORY_APP "APP"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "log", NULL, 0, 0 },
    { OPT_ARG_OPTIONAL, "db" , NULL, 0, 0 },
};

typedef enum AppState 
{
    APP_STATE_MENU,
    APP_STATE_EXIT
} AppState;

typedef struct AppData
{
    AppState state;
    DiffTree* dtree;
    int exit;
} AppData;

typedef void (*AppCallback) (AppData*);

typedef struct App 
{
    AppState state;
    AppCallback callback;
} App;

void app_callback_menu       (AppData* adata);
void app_callback_exit       (AppData* adata);

static App app_state[] =
{
    { APP_STATE_MENU,       app_callback_menu       },
    { APP_STATE_EXIT,       app_callback_exit       }
};

void clear_screen();

int main(int argc, char* argv[])
{
    utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts));

    if(!long_opts[0].is_set) {
        return EXIT_FAILURE;
    }

    utils_init_log_file(long_opts[0].arg, LOG_DIR);


    // if(long_opts[1].is_set)
    //     err = fact_tree_fread(&dtree, long_opts[1].arg);
    // else
    //     err = fact_tree_fread(&dtree, "db.txt");
    //
    // if(err != FACT_TREE_ERR_NONE) {
    //     UTILS_LOGE(LOG_CATEGORY_APP, "%s", fact_tree_strerr(err));
    // }
    
    // AppData appdata = {
    //     .state = APP_STATE_MENU,
    //     .dtree = &dtree,
    //     .exit = 0
    // };

    // while(!appdata.exit) {
    //     for(size_t i = 0; i < SIZEOF(app_state); ++i) {
    //         if(appdata.state == app_state[i].state) {
    //             clear_screen();
    //             printf("EXYST© expert system [build v0.1]\n\n");
    //             app_state[i].callback(&appdata);
    //             break;
    //         }
    //     }
    // }
    //
    // fact_tree_dtor(&dtree);

    utils_end_log();

    return EXIT_SUCCESS;
}

void app_callback_menu(AppData* adata)
{
    // printf("Modes\n"
    //        "1. Guess\n"
    //        "2. Load from file\n"
    //        "3. Save to file\n"
    //        "4. Get defition\n"
    //        "5. Get difference\n"
    //        "6. Exit\n"
    //        "Enter mode number: "
    // );
    //
    // int input = 0;
    // scanf("%d", &input);
    // clear_stdin_buffer();
    //
    // switch(input) {
    //     case 1:
    //         adata->state = APP_STATE_GUESS;
    //         break;
    //     case 2:
    //         adata->state = APP_STATE_LOAD;
    //         break;
    //     case 3:
    //         adata->state = APP_STATE_SAVE;
    //         break;
    //     case 4:
    //         adata->state = APP_STATE_DEFINITION;
    //         break;
    //     case 5:
    //         adata->state = APP_STATE_DIFFERENCE;
    //         break;
    //     case 6:
    //         adata->state = APP_STATE_EXIT;
    //         break;
    //     default:
    //         break;
    // }
}

void app_callback_exit(AppData* adata)
{
}
