import re
from collections import defaultdict
from statistics import mean



def parse_results(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    # each element in setups is a dict
    setups = {} # NFS, 1 SERVER, 2 SERVER, 3 SERVER
    setups_list = content.split('*** ')
    results = {}

    for setup in setups_list:
        test_num = 0
        if not setup.strip():
            continue

        lines = setup.strip().splitlines()
        setup_name = lines[0].strip()
        tests_data = "\n".join(lines[1:])
        setups[setup_name] = {}

        tests = re.split(r'### (Test \d+)', tests_data)
        if tests == ['']:
            continue
        # print("Tests found:", tests)
        # print("Test data:", tests_data)
        # re.split gives ['', 'Test 1', 'data1', 'Test 2', 'data2', ...]

        # setup_results = defaultdict(lambda: defaultdict(list))

        for i in range(1, len(tests), 2):
            test_num += 1
            test_name = tests[i]
            test_content = tests[i+1].strip().splitlines()

            # print("Test content:", test_content)
            # exit()
            for i in range(0, len(test_content), 6):
                test_case = test_content[i].strip()
                values = [float(x) for x in test_content[i+1:i+6]]
                if test_case in ['Write ST', 'Write MT', 'Read ST', 'Read MT']:
                    if test_case not in setups[setup_name]:
                        setups[setup_name][test_case] = values
                    else:
                        setups[setup_name][test_case] = [a + b for a, b in zip(values, setups[setup_name][test_case])]
                else:
                    print("Unknown test case:", test_case)
                    exit()
                # print (setups[setup_name])

        # Compute averages
        # print("Test number:", test_num)
        # print("Setup name:", setup_name)
        for category in ['Write ST', 'Write MT', 'Read ST', 'Read MT']:
            setups[setup_name][category] = [x / test_num for x in setups[setup_name][category]]

    return setups

# Example usage:
result_data = parse_results("test_results.txt")
for setup, categories in result_data.items():
    print(f"Setup: {setup}")
    for cat, avg in categories.items():
        print(f"  {cat}: {[f"{x:.3f}" for x in avg]}")
