#ifndef NMOS_FLOWCOMPATIBILITY_BEHAVIOUR_H
#define NMOS_FLOWCOMPATIBILITY_BEHAVIOUR_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        void streamcompatibility_behaviour_thread(nmos::node_model& model, slog::base_gate& gate);
    }
}

#endif
