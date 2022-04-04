#pragma once

#include "redis_connection.hpp"
#include "redis_execute.hpp"

namespace Redis {

    class RedisTransaction {
    private:
        RedisConnection& con;
        bool holding_transaction{false};

    public:
        RedisTransaction(RedisConnection& con) : con{con} {}

        bool begin_transaction(bool using_watch=false) {
            if (using_watch) {
                execute(con, "WATCH").parse<std::string>();
            }
            std::string resp{execute(con, "MULTI").parse<std::string>()};
            if (resp != "+OK") {
                LOG_ERROR("Could not start transaction: {0}", resp);
                return false;
            }

            LOG_INFO("Started transaction");
            holding_transaction = true;
            return true;
        }
        
        void end_transaction() {
            if (!holding_transaction) {
                LOG_ERROR("No active transaction to end!");
                return;
            }

            std::string resp{execute(con, "EXEC").parse<std::string>()};
            LOG_INFO(resp);
        }

        void discard_transaction() {
            if (!holding_transaction) {
                LOG_ERROR("No active transaction to discard!");
                return;
            }

            std::string resp{execute(con, "DISCARD").parse<std::string>()};
            LOG_INFO(resp);
        }
    };
}