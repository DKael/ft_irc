#include <iostream>
#include <map>
#include <string>

using namespace std;

class User {
 private:
  string name;

 public:
  User(const string& _name) : name(_name) {
    cout << "User " << name << " is created!\n";
  }
  User(const User& _origin) : name(_origin.name) {
    cout << name << " copy constructor called!\n";
  }
  ~User() { cout << "User " << name << " is destroyed!\n"; }
  User& operator=(const User& _origin) {
    name = _origin.name;
    cout << name << " assignment operator called!\n";
    return *this;
  }
  const string& get_name(void) { return name; }
};

int main() {
  pair<std::string, User> tmp1("test1", User("user1"));
  pair<std::string, User> tmp2("test2", User("user2"));

  User user3("user3");

  pair<std::string, User&> tmp3("test3", user3);

  cout << &user3 << ' ' << &(tmp3.second) << '\n';

  map<string, User> map1;
  map1.insert(tmp1);
  map1.insert(tmp3);

  cout << &(tmp1.second.get_name()) << ' ' << &(tmp3.second.get_name()) << '\n';
  map<string, User>::iterator it = map1.begin();
  for (; it != map1.end(); it++) {
    cout << it->second.get_name() << ' ' << &(it->second.get_name()) << ' ';
  }
  cout << '\n';
}