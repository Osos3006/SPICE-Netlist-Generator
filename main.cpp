#include<iostream>
#include<string>
#include<vector>
#include<stack>
using namespace std;

// Global variables
int mos_count = 0; // Index used for naming
string not_list = "" , or_list="" , and_list =""; // string for each gate that records its netlist
int not_nodes = 0 , or_nodes = 0 , and_nodes = 0 ; // counters to name the nodes that represents the output of each gate

// This function associates precedence with operators
int precedence(char a)
{
    if (a == '\'')
        return 2;
    else if (a == '&')
        return 1;
    else if (a == '|')
        return 0;
    else
        return -1;
}

// This function determines whether a character is an operator or not
bool isOperator(char a)
{
    if (a == '&' || a == '|' || a == '\'')
        return true;
    return false;
}

// This function converts the infix expression to postfix expression to ease its processing
string infix_to_postfix(string infix)
{
    string postfix = "";
	int i = 0, n = infix.length();
	char e='%', c='-', current;
    stack<char> conversion;
	while (i < n)
	{
        current = infix[i];
		if (!isOperator(current) && current != '(' && current != ')')
			postfix += current;
		else if (current == '(')
			conversion.push(current);
		else if (current == ')')
		{
            e = conversion.top();
            while ((e != '(') && (!conversion.empty()))
            {
                e = conversion.top();
                conversion.pop();
                postfix += e;
                e = conversion.top();
            }
            e = conversion.top();
            conversion.pop();
		}
		else
		{
            if (!conversion.empty())
                e = conversion.top();
            while ((!conversion.empty()) && (precedence(infix[i]) <= precedence(e)))
            {
                c = conversion.top();
                conversion.pop();
                postfix += c;
                if (!conversion.empty())
                    e = conversion.top();
            }
            conversion.push(infix[i]);
		}
		i++;
	}
		while (!conversion.empty())
		{
            e = conversion.top();
            conversion.pop();
			postfix += e;  
		}
	return postfix;
}

// This function is responsible for creating the inverter PUN and PDN
string Process_not(string x , string & output_str = not_list)
{
    output_str = output_str + "M" + to_string(mos_count) + ' ' + "NOT" + to_string(not_nodes) + ' ' + x + ' ' + "vdd " + "vdd " + "PMOS" + '\n';
    mos_count++;
    output_str = output_str + "M" + to_string(mos_count) + ' ' + "NOT" + to_string(not_nodes) + ' ' + x + ' ' + '0' + ' ' + '0' + ' ' + "NMOS" + '\n';
    mos_count++;
    not_nodes++;
    return "NOT" + to_string(not_nodes - 1);
}

// This function is resposible for creating the PUN and PDN for both NAND and AND (by adding an inverter) gates.
string process_and(vector<string>elements, bool invert)
{
    for (int i = 0; i < elements.size(); i++)
	{
		and_list = and_list + "M" + to_string(mos_count) + ' ' + "AND" + to_string(and_nodes) + ' ' + elements[i] + ' ' + "vdd vdd " + " PMOS" + '\n';
		mos_count++;
		if (i == 0)
			and_list = and_list + "M" + to_string(mos_count) + ' ' + "AND" + to_string(and_nodes) + ' ' + elements[i] + ' ' + "M" + to_string(mos_count + 2) + "NODE " + "M" + to_string(mos_count + 2) + "NODE " + "NMOS" + '\n';
		else
			if (elements[i] == elements.back())
				and_list = and_list + "M" + to_string(mos_count) + ' ' + "M" + to_string(mos_count) + "NODE" + ' ' + elements[i] + ' ' + "0 0 " + " NMOS" + '\n';
			else
				and_list = and_list + "M" + to_string(mos_count) + ' ' + "M" + to_string(mos_count) + "NODE" + ' ' + elements[i] + ' ' + "M" + to_string(mos_count + 2) + "NODE" + "M" + to_string(mos_count + 2) + "NODE" + " NMOS" + '\n';
		mos_count++;
	}
	string out = "AND" + to_string(and_nodes);
	if (invert) 
        out = Process_not(out, and_list); // flag to invert the NAND into AND to avoid double inversion 
	and_nodes++;
	return out;
}

//This function is resposible for creating the PUN and PDN for both NOR and OR (by adding an inverter) gates.
string process_or(vector<string>elements, bool invert)
{
    for (int i = 0; i < elements.size(); i++)
	{
		or_list = or_list + "M" + to_string(mos_count) + ' ' + "or" + to_string(or_nodes) + ' ' + elements[i] + ' ' + "0 0" + " NMOS" + '\n';
		mos_count++;
		if (i == 0)
			or_list = or_list + "M" + to_string(mos_count) + ' ' + "M" + to_string(mos_count + 2) + "NODE" + ' ' + elements[i] + ' ' + "vdd vdd " + "PMOS" + '\n';
		else
			if (elements[i] == elements.back())
				or_list = or_list + "M" + to_string(mos_count) + ' ' + "or" + to_string(or_nodes) + ' ' + elements[i] + ' ' + "M" + to_string(mos_count) + "NODE " + "M" + to_string(mos_count) + "NODE" + " PMOS" + '\n';
			else
				or_list = or_list + "M" + to_string(mos_count) + ' ' + "M" + to_string(mos_count + 2) + "NODE" + ' ' + elements[i] + ' ' + "M" + to_string(mos_count) + "NODE " + "M" + to_string(mos_count) + "NODE" + " PMOS" + '\n';
		mos_count++;
	}
	string out = "OR" + to_string(or_nodes);
	if (invert)
        out = Process_not(out, or_list); //flag to invert the NOR into OR to avoid double inversion
	or_nodes++;
	return out;
}

string replace(string func, string sub, string str)
{
	std::size_t pos = func.find(sub);
	while (pos != std::string::npos)
    {
		func.replace(pos, sub.length(), str);
		pos = func.find(sub);
	}
	return func;
}

// Generate the NETLIST function
string produce_final_output(string postfix , string output_label)
{
	//x=(a&b)|(c'&(d|e')); y=f'&g'; z=h&(i|(j&m'));
    // Code here
    string netlist;
    stack<string> s;
    string temp = "";
    int successor_operands = 0;
    bool add_inverter = false;
    vector <string> arguments;
    //cout << "postfix " << postfix <<endl;
    //Parsing the postfix expression 
    for ( int i = 0 ; i < postfix.length(); i++)
    {
        if (postfix[i] != '\'' && postfix[i] != '&' && postfix[i] != '|')
        {
            temp = postfix[i];
            s.push(temp);
        }
        else if (postfix[i] == '&')
        {
            if(!s.empty())
            {
                add_inverter=true;
                successor_operands=2;
                while (i < postfix.length() - 1 && postfix[i] == postfix[i + 1])
				{
					i++;
					successor_operands++;
				}
                if (i < postfix.length() - 1 && postfix[i + 1] == '\'')
				{
					i++;
					add_inverter = false; // NAND Gate
				}
                while (successor_operands != 0)
				{
					if (s.empty())
					{
						cout << "Error in the counter of successor operands, post expression is not correct.\n";
						break;
					}
					else
					{
						temp = s.top();
						s.pop();
						arguments.insert(arguments.begin(), temp);
					}
					successor_operands--;
				}
                temp = process_and(arguments, add_inverter);
                // for ( int i = 0 ; i < arguments.size () ; i++)
                // cout << "argument" << arguments[i] <<endl;
				arguments.clear();
				netlist += and_list;
				and_list = "";
				netlist += "\n";
				s.push(temp);
            }
            else
            {
                cout << "There is an error in the postfix string before an -&- \n";
				break;
            }
        }
        else if (postfix[i] == '|')
        {
            if (!s.empty())
            {
                add_inverter=true;
                successor_operands = 2 ;
                while (i < postfix.length() - 1 && postfix[i] == postfix[i + 1])
                {
                    i++;
                    successor_operands++;
                }
                if (i < postfix.length() - 1 && postfix[i + 1] == '\'')
                {
                    i++;
                    add_inverter = false; // NOR Gate
                }
                while (successor_operands != 0)
                {
                    if (s.empty())
                    {
                        cout << "Error in the counter of successor operands, post expression is probably wrong\n";
                        break;
                    }
                    else
                    {
                        temp = s.top();
                        s.pop();
                        arguments.insert(arguments.begin(), temp);
                    }
                    successor_operands--;
                }
                temp = process_or(arguments, add_inverter);
        		arguments.clear();
		        netlist += or_list;
		        or_list = "";
		        netlist += "\n";
		        s.push(temp);
            }
            else
            {
                cout << "There is an error in the postfix string before an -|- \n";
			    break;
            }
        }
        else if (postfix[i] == '\'')
	    {
            if (!s.empty())
            {
                temp = s.top();
                s.pop();
                temp = Process_not(temp);
                netlist += not_list;
                not_list = "";
                netlist += "\n";
                s.push(temp);
            }
            else
            {
                cout << "There is an error in the postfix string before an -'- \n";
                break;
            }
        }
	}
	if (!s.empty())
	{
		temp = s.top();
		s.pop();
		netlist = replace(netlist, temp, output_label); // net list for a single operation ( &,/, '). Hence, the output lable is passed to append all the netlists and replace the last with the output label
	}
	return netlist;
}

int main()
{
    string user_input, temp, output_label="out";
    cout << "Please enter the expresssion(s):\n";
    /*
    The input should strictly follow the following format: <output1>=<expression1>; <output2>=<expression2>; ... <outputn>=<expressionn>;
    where each expression:
    1) has no spaces within
    2) followed by exactly one ; and one "␣" 
    3) the last expression should be terminated by ';'
    Example of valid input: y=a&b; x=c|d; 
    Example of invalid input: a&b;
    Example of invalid input: x=a&b y=c';
    Example of invalid input: x=a& b;
    */
    getline(cin, user_input);
    int n = user_input.length(), expression_starting_index = 0;
    vector<string> expressions, cropped_expressions;
    for (int i = 0; i < n; i++)
        if (user_input[i] == ';')
        {
            temp = user_input.substr(expression_starting_index, i - expression_starting_index);
            expressions.push_back(temp);
            cropped_expressions.push_back(temp.substr(2, temp.size() - 2));
            expression_starting_index = i + 2;
        }
    int m = cropped_expressions.size();
    cout << "Expressions:\n";
    for (int i = 0; i < m; i++)
        cout << cropped_expressions[i] << "\t\tCorresponding postfix->\t" << infix_to_postfix(cropped_expressions[i]) << endl;
    cout << "\n\n-------------- Final Output --------------\n\n";
    for (int i = 0; i < m; i++)
    {
        string final_netlist;
        not_list = "" ; or_list = "" ; and_list = ""; 
        mos_count = 0;
        not_nodes = 0; or_nodes = 0; and_nodes = 0; //reinitialize
        cout << "------- " << expressions[i] << " -------\n\n";
       final_netlist = produce_final_output(infix_to_postfix(cropped_expressions[i]), output_label);
       cout << final_netlist << endl;
    }
    return 0;
}
