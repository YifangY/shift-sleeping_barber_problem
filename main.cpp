/*
 * Sleeping barber problem
 * by Yifang Yuan
 * yyuan17@stevens.edu
 * 
 * REQ:https://github.com/thiagowinkler/shift-sleeping_barber_problem
 * 1. Seats number is the only input
 * 2. One thread for barber(three status:sleeping, waking up and working)(infinite)
 * 3. One thread for creating customer by 3 seconds(infinite loop)
 * 4. Threads operation
 *
 * Complier(gcc version 5.4.0)
 * g++ -std=c++11 XXX.cpp -pthread
 *
 * Ref: https://en.wikipedia.org/wiki/Sleeping_barber_problem
 *
 *
 */

#include <iostream>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>

/*
 * 1 One class for the problem
 *
 * 2 One method for creating customer thread
 *
 */

class SleepingBarberProblem{
private:
    /*
     *  1.1 One class method for barber thread and main status output stream
     *  1.2 One class method for customer thread
     *  1.3 Free_seats and barber_mission_list_ are shared by barber and customer.
     *       Their function are similar, just for code readable
     *  1.4 Two condition_variable cust_ready(customet-->>barber),cust_finish_(barber-->>customet)
     *  1.5 haircutting_time_: count the "cutting time"
     *  1.6 servicing_cust_id_ and leaving_cust_id_ to mark the customer id and output result
     *  1.7 last_status_sleep_: output "waker by" or not?
     */

    std::vector<unsigned long> pub_barber_mission_list_;
    unsigned int pub_free_seats_;
    unsigned int haircutting_time_;
    unsigned int last_status_sleep_;
    unsigned long servicing_cust_id_;
    unsigned long leaving_cust_id_;
    std::condition_variable cust_ready_, cust_finish_;
    std::mutex mutex_cv_barber_sleep_, mutex_access_seats_, mutex_stream_output_, mutex_cv_customer_terminating_;

    //Generate random number 1..5, for hair cutting time
    int RandOneToFive() {
        unsigned int loc_result_ = rand() % 4 + 1;
        return loc_result_;
    }

public:
    SleepingBarberProblem(unsigned int &free_seat):pub_free_seats_(free_seat){

    }

// 1.1 method for barber thread
    void Barber() {
    //a. It's infinite loop
        while (true) {
    //b. Process MissionList is empty? Yes, sleep and wait for customer call(cust_ready); No "c"
            std::unique_lock<std::mutex> loc_lock_sleep_(mutex_cv_barber_sleep_);
            while (pub_barber_mission_list_.empty()) {
                std::cout << "\nBarber sleeping.... Waiting room is empty\n";
                last_status_sleep_ = 1;
                cust_ready_.wait(loc_lock_sleep_);
            }
            loc_lock_sleep_.unlock();

    //c. Load the begin of the MissionList. It's the sevicing customer. Lock, update MissionList and FreeSeats, Unlock.
            std::unique_lock<std::mutex> loc_lock_public_var_of_barber_customer(mutex_access_seats_);
            servicing_cust_id_ = pub_barber_mission_list_.at(0);
    //if barber last status is sleep, print waked by...
            if (last_status_sleep_ == 1) {
                std::cout << "Barber waked by " << servicing_cust_id_ << "\n";
                last_status_sleep_ = 0;
            }
            std::vector<unsigned long>::iterator loc_delete_first_ = pub_barber_mission_list_.begin();
            loc_delete_first_ = pub_barber_mission_list_.erase(loc_delete_first_);
            ++pub_free_seats_;
            haircutting_time_ = RandOneToFive();
            loc_lock_public_var_of_barber_customer.unlock();

    //d. Output Baber Status and Wait list... by 1 seconds
            while (haircutting_time_ != 0) {
                mutex_stream_output_.lock();
                std::cout << "Barber cutting the hair of customer " << servicing_cust_id_ << "\n";
                std::cout << "Waiting room: ";
                for (auto loc_wait_customer_id_ : pub_barber_mission_list_) {
                    std::cout << loc_wait_customer_id_  << " ";
                }
                std::cout << "\n";
                mutex_stream_output_.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                --haircutting_time_;
            }
    //e. After hair cutting finished, notify the customer thread to terminate(by id(leaving_cust_id) number)
            leaving_cust_id_ = servicing_cust_id_;
            cust_finish_.notify_all();
            std::cout << "Finish. Customer " << servicing_cust_id_ << " leaving\n";
        }
    }

// 1.2 method for customer thread, load the customer id
    void Customer(unsigned long customer_id_) {
        std::unique_lock<std::mutex> loc_lock_public_var_of_barber_customer(mutex_access_seats_);
    //a. if free seat is 0, terminated
        if (pub_free_seats_ == 0) {
            mutex_stream_output_.lock();
            std::cout << "No more seat. Customer " << customer_id_ << " leaving\n";
            mutex_stream_output_.unlock();
            loc_lock_public_var_of_barber_customer.unlock();

        } else {
    //b. Lock, update MissionList and FreeSeats, notify barber Unlock.
            pub_barber_mission_list_.push_back(customer_id_);
            --pub_free_seats_;
            cust_ready_.notify_one();
            loc_lock_public_var_of_barber_customer.unlock();

    //c. Wait for barber's notification, about haircutting finished
            std::unique_lock<std::mutex> loc_lock_cust_finish_cutting_(mutex_cv_customer_terminating_);
            //Check the id
            while (customer_id_ != leaving_cust_id_) {
                cust_finish_.wait(loc_lock_cust_finish_cutting_);
            }
            loc_lock_cust_finish_cutting_.unlock();
        }
    }
};

// 2 One method for creating customer thread
void CreateCustomer(SleepingBarberProblem *input_problem_) {
    //vector for every thread and index as customer id
    std::vector<std::thread> customer_thread_storage_;
    unsigned long customer_id_and_thread_index_ = 1;
    //Infinite loop
    while(true){
        customer_thread_storage_.push_back(std::thread{&SleepingBarberProblem::Customer, input_problem_, customer_id_and_thread_index_});
        customer_thread_storage_[customer_id_and_thread_index_ - 1].detach();
        ++customer_id_and_thread_index_;
        //add new customer by 3 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}


int main() {
    std::cout << "Please in put seat number(between 1 and 1000)\n";
    unsigned int seat_number_;
    std::cin>>seat_number_;

    //new class for the program
    SleepingBarberProblem *barber_problem_class_= new SleepingBarberProblem(seat_number_);

    //barber thread
    std::thread barber_thread_{&SleepingBarberProblem::Barber, barber_problem_class_};
    barber_thread_.detach();

    //"creating customer threads" thread
    std::thread customer_creating_thread_(CreateCustomer,barber_problem_class_);
    customer_creating_thread_.join();

    return 0;
}



